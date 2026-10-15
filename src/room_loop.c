/* room_loop.c — the room loop's per-tick calls.  See room_loop.h.
 *
 * Transcribed from the asm.  GetTickCount is read exactly where retail reads it (rhost's
 * virtual clock counts calls). */
#include "room_loop.h"

#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "actor_update.h"
#include "char_ai.h"
#include "char_behave.h"
#include "char_physics.h"
#include "engine_calls.h"
#include "entity_fx.h"
#include "entity_move.h"
#include "gimmick.h"
#include "grid_refresh.h"
#include "input.h"
#include "input_dik.h"
#include "char_state.h"
#include "npc_ai.h"

#define IP(p) ((intptr_t)(p))

static int32_t iabs(int32_t v) { return v < 0 ? (int32_t)(0u - (uint32_t)v) : v; }
static void *sysp(uint32_t off) { void *p; memcpy(&p, (const uint8_t *)g_eng.sys + off, sizeof p); return p; }

void room_input_update(wl_room *r)
{
    for (uint16_t j = 0; j < r->nin; j++) {
        input_mgr *m = &r->in[j];
        int32_t old[11];
        for (int k = 0; k < 11; k++) {
            old[k] = m->axis_held[k];
            m->axis_held[k] = 0;
        }
        if (m->src_kb != 0) input_dik_produce(m, sysp(0x2878));
        for (uint32_t k = 0; k < 2; k++) {
            void *joy = sysp(0x287c + 4 * k);
            if (m->src_joy[k] != 0 && joy != NULL) g_eng.h468b10(m, IP(joy), 0);
        }
        uint32_t now = g_eng.tick();
        for (int k = 0; k < 11; k++) {
            if (m->axis_held[k] == 0) m->axis_held_b[k] = 0;
            else if (old[k] == 0) m->axis_held_b[k] = (int32_t)now;
        }
    }
}

int room_char_step(wl_char *c, int32_t cut)
{
    wl_body *b = c->base.body;
    int32_t y;
    int gr = entity_move_vertical(&c->base, b, 1, b->drop, 0, &y, NULL) == 0;
    int32_t mode = c->f200;
    if (cut == 0 && c->base.coll_off != 0) mode = 1;
    if (mode == 0) {
        if (cut == 0) return char_ai(c, b, gr);
    } else if (mode == 1) return char_behave(c, cut, b, gr);
    return gr;
}

void room_actor_step(wl_actor *a)
{
    wl_body *b = a->base.body;
    if (a->cooldown > 0) a->cooldown--;
    if (a->f2e8 != 0) {
        if (a->f2ec != 0) a->f2ec--;
        else {
            a->f2ec = 16;
            int32_t box[4];
            box[0] = b->w / 2 + b->x - 32000;
            box[2] = 64000;
            box[3] = 48000;
            box[1] = b->y + (b->h - b->margin) / 2 + b->margin - 24000;
            a->base.faction = char_in_rect(0, box, 0) != 0 ? 3 : 2;
        }
    }
    int32_t slow = b->slow, cf = b->c_flag;
    actor_update(a, a->base.body, a->base.body, 0, 0);
    entity_splash(&a->base, b, slow, cf);   /* inlined in retail (the same records) */
    entity_fx_tick(&a->base, b, 0);
    if (a->multipart != 0)
        for (uint16_t k = 1; k < a->base.nbody; k++) actor_update(a, &a->base.body[k - 1], &a->base.body[k], 1, k);
    if (a->base.snd != NULL)
        for (uint16_t i = 0; i < a->base.nsnd; i++) entity_snd_update(&a->base, i);
}

void room_grid_anim(uint8_t *grid)
{
    int32_t f; memcpy(&f, grid + 0x28, 4);
    if (f != 0) {
        f = 0;
        memcpy(grid + 0x28, &f, 4);
        grid_refresh(grid, 0);
    }
    uint16_t ph, big; memcpy(&ph, grid + 0x26, 2);
    if (ph < 6) {
        ph++;
        memcpy(grid + 0x26, &ph, 2);
        return;
    }
    memcpy(&big, grid + 0x24, 2);
    big++;
    ph = 0;
    memcpy(grid + 0x24, &big, 2);
    memcpy(grid + 0x26, &ph, 2);
    if (big >= 6) { big = 0; memcpy(grid + 0x24, &big, 2); }
}

int room_party_down(const wl_room *r)
{
    int up = 0;
    for (int k = 0; k < 8; k++) {
        const wl_pslot *s = r->slot[k];
        if (s->active != 1 || s->id == 100000104) continue;
        const wl_char *c = s->ch;
        if (c->base.alive == 1 && c->base.body->active == 1 && c->party->busy == 0) up = 1;
    }
    return up ? 0 : 4;
}

void room_lead(wl_roomloop *rl, const wl_char *c, const wl_char *partner, int32_t mode)
{
    int32_t t = -30000;
    if (mode == 1) {
        if (c != NULL && partner != NULL) {
            const wl_body *b = c->base.body, *p = partner->base.body;
            t = p->w / 2 - b->w / 2 - b->x + p->x - 30000;
            if (t < -52000) t = -52000;
            else if (t > -8000) t = -8000;
        }
    } else if (mode == 2) {
        if (c != NULL) {
            if (c->base.nbody == 0) {
                char buf[256];
                snprintf(buf, sizeof buf, "%d", c->base.code);
                g_eng.h408b90(g_eng.log, IP(g_eng.data(0x85451cu)), 0, IP(buf));   /* "GetHeadLeftRight State Count Over." */
            }
            t = c->base.body->facing == 1 ? -8000 : -52000;
        }
    } else return;
    int32_t cur = rl->lead, step = iabs(cur - t) / 40;
    if (cur < t) rl->lead = cur + step <= t ? cur + step : t;
    else rl->lead = cur - step >= t ? cur - step : t;
}

/* a timed-sound record (sys+0xaa8 -> +0x8, 24 B) */
typedef struct tsound {
    void    *buf;     /* +0x00 */
    int32_t  vol;     /* +0x04 */
    uint32_t dur;     /* +0x08  ms it plays */
    uint32_t t0;      /* +0x0c */
    int32_t  state;   /* +0x10  1 start, 2 playing, 3 fading */
    int32_t  fade;    /* +0x14  volume lost per tick */
} tsound;

void timed_sounds_tick(void *sys)
{
    const uint8_t *s = sys;
    void *bgm; memcpy(&bgm, s + 0x2884, sizeof bgm);
    if (bgm != NULL) g_eng.h5bc0c0(bgm);
    uint8_t *l; memcpy(&l, s + 0xaa8, sizeof l);
    if (l == NULL) return;
    tsound *rec; memcpy(&rec, l + 8, sizeof rec);
    if (rec == NULL) return;
    uint32_t now = g_eng.tick();
    uint16_t n; memcpy(&n, l + 4, 2);
    for (uint16_t i = 0; i < n; i++, memcpy(&n, l + 4, 2)) {
        memcpy(&rec, l + 8, sizeof rec);
        tsound *t = &rec[i];
        if (t->buf == NULL) continue;
        switch (t->state) {
        case 1:
            t->t0 = g_eng.tick();
            t->state = 2;
            g_eng.dsb_volume_5bb870(t->buf, t->vol);
            g_eng.dsb_play_5bb8c0(t->buf);
            break;
        case 2:
            if (now - t->t0 > t->dur) {
                t->state = 3;
                t->fade = 200;
            }
            break;
        case 3: {
            int32_t v = t->vol - t->fade;
            if (v < -10000) v = -10000;
            t->vol = v;
            g_eng.dsb_volume_5bb870(t->buf, v);
            if (t->vol > -10000) break;
            void *b = t->buf;
            if (b != NULL) {
                g_eng.dsb_release_5bb3a0(b);
                g_eng.op_delete(b);
            }
            memset(t, 0, sizeof *t);
            break;
        }
        default: break;
        }
    }
}

/* ── the encounter pass ────────────────────────────────────────────────────── */

static uint16_t sys16(uint32_t off) { uint16_t v; memcpy(&v, (const uint8_t *)g_eng.sys + off, 2); return v; }
static void sys16w(uint32_t off, uint16_t v) { memcpy((uint8_t *)g_eng.sys + off, &v, 2); }
static uint32_t sys32r(uint32_t off) { uint32_t v; memcpy(&v, (const uint8_t *)g_eng.sys + off, 4); return v; }
static void sys32w(uint32_t off, uint32_t v) { memcpy((uint8_t *)g_eng.sys + off, &v, 4); }
static int32_t party32(const wl_char *c, uint32_t off) { int32_t v; memcpy(&v, (const uint8_t *)c->party + off, 4); return v; }
/* the level sum the threat compares (party +0xd8 + +0xe0 + +0xe4) */
static int32_t level_sum(const wl_char *c) { return party32(c, 0xd8) + party32(c, 0xe0) + party32(c, 0xe4); }
static int hostile(int32_t a, int32_t b) { return a != 0 && b != 0 && (a == 3 || b == 3 || a != b); }
/* a live, standing, not-busy character of the 32 */
static int engaged_ok(const wl_char *o, int32_t ai_only)
{
    if (o->base.alive != 1) return 0;
    const wl_body *b = o->base.body;
    if (b->active != 1 || b->f7e != 0 || o->party->busy != 0) return 0;
    return ai_only == 0 || o->f200 == 0 || o->f14288 == 1;
}
static int32_t threat(int32_t d)
{
    switch (d) {
    case -4: return 4;
    case -3: case -2: return 6;
    case -1: case 0: case 1: return 10;
    case 2: return 15;
    default: return d >= 3 ? 40 : 0;
    }
}

int foes_near(const wl_char *c, int32_t ranged, int32_t rx, int32_t ry, uint16_t *score, int32_t ai_only,
              const wl_char *ref, int32_t lvl)
{
    int32_t sc = 0;
    int found = 0;
    if (ref == NULL) ref = c;
    for (uint16_t i = 0; i < 32; i++) {
        const wl_char *o = g_world->chars[i];
        if (o->base.alive != 1) continue;
        const wl_body *ob = o->base.body;
        if (ob->active != 1 || ob->f7e != 0 || o->party->busy != 0) continue;
        if (!hostile(c->base.faction, o->base.faction)) continue;
        if (ai_only != 0 && o->f200 != 0 && o->f14288 != 1) continue;
        if (ranged != 0) {
            const wl_body *b = c->base.body;
            int32_t dx = iabs(b->w / 2 + b->x - part_center_x(&o->base, 0));
            int32_t oy = part_center_y(&o->base, 0);
            int32_t dy = iabs(part_center_y(&c->base, 0) - oy);
            if (dx >= rx || dy >= ry) continue;
        }
        found = 1;
        if (score == NULL) return 1;
        sc += threat(level_sum(o) - level_sum(ref) - lvl);
    }
    if (score != NULL) *score = (uint16_t)sc;
    return found;
}

int party_foes_near(const wl_room *room, const wl_char *ref, uint16_t *best, int32_t rx, int32_t ry, int32_t ai_only,
                    uint16_t min, int32_t lvl)
{
    uint16_t top = 0;
    const wl_char *who = NULL;
    for (uint16_t k = 0; k < 8; k++) {
        const wl_pslot *s = room->slot[k];
        if (s->active != 1 || s->id == 100000104) continue;
        const wl_char *m = s->ch;
        if (m == NULL) continue;
        const wl_char *r = ref != NULL ? ref : m;
        int32_t sc = 0;
        int found = 0;
        for (uint16_t i = 0; i < 32; i++) {
            const wl_char *o = g_world->chars[i];
            if (!engaged_ok(o, 0) || !hostile(m->base.faction, o->base.faction)) continue;
            if (ai_only != 0 && o->f200 != 0 && o->f14288 != 1) continue;
            int32_t ox = part_center_x(&o->base, 0);
            int32_t dx = iabs(part_center_x(&m->base, 0) - ox);
            int32_t oy = part_center_y(&o->base, 0);
            int32_t dy = iabs(part_center_y(&m->base, 0) - oy);
            if (dx >= rx || dy >= ry) continue;
            found = 1;
            sc += threat(level_sum(o) - level_sum(r) - lvl);
        }
        if (!found || (uint16_t)sc < min) continue;
        if (best == NULL) return 1;
        if (top < (uint16_t)sc || who == NULL) {
            top = (uint16_t)sc;
            who = m;
        }
    }
    if (best != NULL) *best = top;
    return who != NULL;
}

int pslot_ok(const wl_pslot *s, int32_t any, int32_t inactive)
{
    if (s->active == 1) return s->id != 100000104 || any != 0;
    if (s->active == 2) return inactive;
    return 0;
}

uint16_t bgm_pick(const wl_roomloop *rl)
{
    if (rl->bgm_fixed != 0) return rl->bgm_id;
    const uint8_t *ri = g_world->roominfo;
    int32_t kind; memcpy(&kind, ri + 0x110, 4);
    uint16_t r = 0, ov;
    int var = 0, crowd = 0;
    switch (kind) {
    case 1: case 8: r = 1; var = 2; crowd = 1; break;
    case 2: r = 34; break;
    case 3: r = 2; break;
    case 4: case 9: case 10: r = 7; var = 1; break;
    case 5: case 6: case 7: r = 37; var = 1; break;
    case 11: case 12: r = 8; var = 1; break;
    case 13: case 14: if (rl->fd4 != 61) { r = 33; var = 2; } break;
    case 15: case 16: r = 39; var = 1; break;
    case 17: r = 36; break;
    case 18: case 19: if (rl->fd4 == 50 || rl->fd4 == 60) r = 34; break;
    default: break;
    }
    memcpy(&ov, ri + 0x144, 2);
    if (ov != 0) r = ov;
    const wl_room *room = g_world->room;
    if (crowd && room != NULL) {
        const wl_pslot *ld = room->leader;
        if (ld != NULL && ld->id == 100000103) r = 9;
        int n = 0;
        for (int k = 0; k < 8; k++) {
            const wl_pslot *s = room->slot[k];
            if (s->active != 1) continue;
            const wl_char *c = s->ch;
            if (c->base.alive == 1 && c->base.body->active == 1 && c->party->busy == 0) n++;
        }
        if (n >= 4) r = 10;
    }
    if (var == 1) {
        if (rl->fd4 == 40) r = 46;
        else if (rl->fd4 == 50 || rl->fd4 == 60) r = 43;
    } else if (var == 2) {
        if (rl->fd4 == 40) r = 45;
        else if (rl->fd4 == 50 || rl->fd4 == 60) r = 43;
    }
    switch (g_world->f2770) {
    case 1: return 40;
    case 2: return 41;
    case 3: return 42;
    default: return r;
    }
}

/* the top threat score among the party's members (score >= 10 counts), lvl offset `lvl` */
static uint16_t party_top(const wl_room *room, const wl_char *ref, int32_t lvl)
{
    uint16_t top = 0, sc;
    const wl_char *who = NULL;
    for (uint16_t k = 0; k < 8; k++) {
        const wl_pslot *s = room->slot[k];
        if (!pslot_ok(s, 0, 0) || s->ch == NULL) continue;
        if (!foes_near(s->ch, 1, 56000, 24000, &sc, 0, ref, lvl) || sc < 10) continue;
        if (top < sc || who == NULL) { top = sc; who = s->ch; }
    }
    return top;
}

void encounter_tick(wl_roomloop *rl, wl_room *room, int32_t quiet)
{
    int changed = 0, force = 0;
    if (room == NULL || room->leader == NULL) return;
    const wl_char *lc = room->leader->ch;
    if (lc == NULL) return;
    int32_t m = g_world->f2770;
    if (m > 0 && m <= 2) {
        sys16w(0x28a0, 20);
        sys32w(0x28a4, (uint32_t)g_world->f2770);
    }
    uint16_t st = lc->party->f466;
    uint16_t sc;
    if (g_world->f2770 == 3) {
        if (st == 0) {
            g_world->f277c = 0;
            g_world->f2770 = 0;
            changed = 1;
        }
    } else if (st != 0 && g_world->f277c != 0) {
        uint16_t b1 = party_top(room, lc, 0), b2 = party_top(room, lc, 3);
        if (b1 >= 50 && b2 < 40 && b2 >= 20) {
            uint32_t t = sys32r(0x28a8);
            if (t == 0 || g_eng.tick() - t > 300000) {
                sys32w(0x28a8, g_eng.tick());
                g_world->f2770 = 3;
                changed = 1;
            }
        }
    }
    g_world->f277c = 0;
    m = g_world->f2770;
    if (m == 1) {
        if (party_top(room, lc, 0) >= 40) {
            sys16w(0x28a0, 0);
            changed = 1;
            g_world->f2770 = 2;
        }
    } else if (m <= 1 || m > 3) {
        /* roaming: is a foe near? */
        g_world->f2774 = 0;
        int k;
        for (k = 0; k < 8; k++) {
            const wl_pslot *s = room->slot[k];
            if (pslot_ok(s, 0, 0) && s->ch != NULL && foes_near(s->ch, 1, 28000, 16000, &sc, 1, lc, 0) && sc >= 10) break;
        }
        if (k == 8) {
            if (party_foes_near(room, lc, NULL, 64000, 32000, 0, 0, 0)) g_world->f2774 = 1;
        } else {
            g_world->f2774 = 1;
            uint16_t top = party_top(room, lc, 0);
            if (g_world->room != NULL) g_eng.h409520(g_world->room, top >= 40 ? 22 : 21, 0, 0, 1, 0, 0, 1, 1, 0);
            g_world->f2770 = top >= 40 ? 2 : 1;
            changed = 1;
            g_world->f2778 = 50;
        }
        uint16_t h = sys16(0x28a0);
        if (h > 0) sys16w(0x28a0, (uint16_t)(h - 1));
        goto bgm;
    }
    /* in battle */
    g_world->f2774 = 1;
    if (g_world->scene_kind == 2) {
        g_world->f2770 = 0;
        changed = 1;
        goto bgm;
    }
    if (g_world->f2778 > 10 && g_world->f2778 < 50) {
        int i;
        for (i = 0; i < 32; i++) {
            const wl_char *o = g_world->chars[i];
            if (char_targetable(o, 0, 0) && hostile(lc->base.faction, o->base.faction)) break;
        }
        if (i == 32) g_world->f2778 = 10;
    }
    for (int k = 0; k < 8; k++) {
        const wl_pslot *s = room->slot[k];
        if (pslot_ok(s, 0, 0) && s->ch != NULL && foes_near(s->ch, 1, 56000, 24000, &sc, 0, lc, 0)) {
            g_world->f2778 = 50;
            goto bgm;
        }
    }
    if (g_world->f2778 != 0) g_world->f2778--;
    else {
        g_world->f2770 = 0;
        changed = 1;
    }
bgm:
    if (g_world->f2770 == 0) {
        if (!changed) {
            if (bgm_pick(rl) == sys16(0x2888)) {
                rl->bgm_wait = 10;
                return;
            }
            if (rl->bgm_wait != 0) {
                rl->bgm_wait--;
                return;
            }
            force = 1;
        }
    } else if (!changed) return;
    if (quiet != 0) return;
    int32_t flag = 0, fade = 0;
    if (!force) switch (g_world->f2770) {
        case 0: flag = 1; fade = 20; break;
        case 1: case 2:
            if (sys16(0x28a0) != 0 && (uint32_t)g_world->f2770 == sys32r(0x28a4)) { flag = 1; fade = 200; }
            break;
        default: break;
        }
    g_eng.h40a5d0(g_eng.sys, bgm_pick(rl), 1, flag, fade, 1);
}
