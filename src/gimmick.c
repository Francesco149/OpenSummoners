/* gimmick.c — FUN_0059afd0 / FUN_0059b170 / FUN_0040afe0 / FUN_0040be30 and their helpers.
 * See gimmick.h.
 *
 * Transcribed from the asm.  0x59b170, 0x40afe0 and 0x40be30 each inline the per-code
 * effect of a switch; they differ only in how the two bridges sound (0x407ba0 one-shot vs
 * a pool voice placed by 0x40bb90) and in 0x40afe0 expanding the 70220 shot emitters
 * inline — switch_effect() takes the inliner.  Quotients truncate toward zero as retail's
 * multiply-and-shift sequences do. */
#include "gimmick.h"

#include <stddef.h>
#include <string.h>

#include "actor_ops.h"
#include "actor_update.h"
#include "char_integrate.h"
#include "char_physics.h"
#include "char_state.h"
#include "engine_calls.h"
#include "entity_life.h"
#include "event.h"
#include "input.h"
#include "npc_ai.h"
#include "particle_spawn.h"
#include "rng.h"
#include "sound_fx.h"

#define IP(p) ((intptr_t)(p))

static int32_t rnd(void) { return (int32_t)rng_rand(); }
static int32_t mul(int32_t a, int32_t b) { return (int32_t)((uint32_t)a * (uint32_t)b); }
static int32_t sfx_volume(void) { return (**g_eng.keycfg)->sfx_volume; }
static void *sysp(uint32_t off) { void *p; memcpy(&p, (const uint8_t *)g_eng.sys + off, sizeof p); return p; }
static void put32(uint8_t *p, uint32_t off, int32_t v) { memcpy(p + off, &v, 4); }
static wl_pslot *char_slot(const wl_char *c) { return (wl_pslot *)(intptr_t)c->fc794; }

/* is character `c` its room's leader? (the 0x5567a0 / 0x5568e0 check) */
static int is_leader(const wl_char *c)
{
    const wl_pslot *s = char_slot(c);
    if (s == NULL) return 0;
    const wl_pslot *l = s->room->leader;
    return c == (l != NULL ? l->ch : NULL);
}

/* the overlap 0x59b170 tests between part 0 of actor `a` and of character `pl` */
static int overlaps(const wl_actor *a, const wl_char *pl)
{
    return part_left(&a->base, 0) <= part_right(&pl->base, 0) && part_right(&a->base, 0) > part_left(&pl->base, 0) &&
           char_part_top((const wl_char *)a, 0) <= part_bottom(&pl->base, 0) &&
           part_bottom(&a->base, 0) > char_part_top(pl, 0);
}

/* ── small helpers ─────────────────────────────────────────────────────────── */

void actor_unlist_all(wl_actor *a)
{
    wl_list_remove(g_world->list_2798, &a->base);
    entity_unlist(&a->base);
}

wl_entity *particle_find(const wl_entity *owner, int32_t code)
{
    for (uint16_t i = 0; i < 0x400; i++) {
        wl_particle *p = (wl_particle *)g_world->particles[i];
        if (p->base.alive != 0 && p->owner == owner && (p->base.code == code || code == 0)) return &p->base;
    }
    return NULL;
}

uint32_t world_next_serial(wl_world *w)
{
    uint32_t v = (uint32_t)w->action_serial;
    w->action_serial = v >= 0xffffffu ? 0 : (int32_t)(v + 1);
    return v + 1;
}

void switch_glow(wl_actor *a, int32_t v, int32_t burst)
{
    int32_t code = a->f2e4 == 1 ? 100135 : 100134;
    wl_body *b = a->base.body;
    if (v == 0) {
        if (particle_find(&a->base, 0) == NULL) particle_spawn(&a->base, &a->base, code, b, 0, 0, 99, 1, 0, 0, 2, 1, 0, 20);
    } else if (v == 1) {
        if (particle_find(&a->base, 0) == NULL) return;
        if (burst != 0)
            for (int i = 8; i != 0; i--) {
                int32_t dx = mul(rnd(), 1600) / 0x8000 - 800;
                particle_spawn(&a->base, NULL, 100103, b, dx, 0, 99, 4, 0, 0, 2, 0, 0, 20);
            }
        g_eng.h556e50(IP(a), 0, 0);
    }
}

int entity_rect_hit(const wl_entity *e, const int32_t *r, uint16_t part)
{
    const wl_body *b = &e->body[part < e->nbody ? part : 0];
    if (b->x > r[0] + r[2]) return 0;
    if (b->x + b->w <= r[0]) return 0;
    if (b->margin + b->y > r[1] + r[3]) return 0;
    return b->h + b->y - 1 > r[1];
}

uint16_t chars_in_rect_count(uint16_t part, const int32_t *r, uint32_t lo, uint32_t hi)
{
    uint16_t n = 0;
    for (int i = 0; i < 32; i++) {
        const wl_char *c = g_world->chars[i];
        if (c->base.alive != 1) continue;
        const wl_body *b0 = c->base.body;
        if (b0->active != 1 || b0->f7e != 0 || c->party->busy != 0 || c->base.faction != 2) continue;
        if (lo != 0 && lo > (uint32_t)c->base.handle) continue;
        if (hi != 0 && hi < (uint32_t)c->base.handle) continue;
        const wl_body *b = &b0[part < c->base.nbody ? part : 0];
        if (b->x > r[0] + r[2]) continue;
        if (b->w + b->x <= r[0]) continue;
        if (b->margin + b->y > r[3] + r[1]) continue;
        if (b->h + b->y - 1 <= r[1]) continue;
        n++;
    }
    return n;
}

int char_in_rect(uint16_t part, const int32_t *r, int32_t leader_only)
{
    for (uint16_t i = 0; i < 32; i++) {
        const wl_char *c = g_world->chars[i];
        int32_t al = c->base.alive;
        if (al == 0 || c->base.rider_heavy == 0 || al != 1) continue;
        const wl_body *b0 = c->base.body;
        if (b0->active != 1 || c->party->busy != 0) continue;
        const wl_body *b = &b0[part < c->base.nbody ? part : 0];
        if (b->x > r[2] + r[0]) continue;
        if (b->w + b->x <= r[0]) continue;
        if (char_part_top(c, part) > r[3] + r[1]) continue;
        if (part_bottom(&c->base, part) <= r[1]) continue;
        if (leader_only == 0) return 1;
        if (c->fc794 == 0) continue;
        if (is_leader(c)) return 1;
    }
    return 0;
}

wl_entity *char_riding(const wl_actor *a, int32_t leader_only, uint16_t *state)
{
    const wl_list *l = g_world->list_2794;
    wl_entity *r = NULL;
    uint16_t n = l->count;
    for (uint16_t i = 0; i < n; i++) {
        wl_entity *e = l->items[i];
        if (e->body->attach != &a->base || e->rider_kind != 1 || e->rider_heavy == 0) continue;
        if (leader_only == 0) { r = e; break; }
        const wl_char *c = (const wl_char *)e;
        if (c->fc794 == 0) continue;
        if (is_leader(c)) { r = e; break; }
    }
    if (state != NULL) {
        *state = 0;
        if (r != NULL) *state = r->body->state;
    }
    return r;
}

int actor_shot(wl_actor *a, wl_body *b, int32_t a3, int32_t power)
{
    uint32_t s = world_next_serial(g_world);
    int32_t pw = mul(a->f2e0[1], power) / 1000;
    return g_eng.h55cfc0(a, 0, 2, 100040, IP(b), 0, 0, 1, a3, 10, pw, 0, (intptr_t)s, 3, 0, 0, 0, 0, 0, 2);
}

void warp_request(wl_actor *a, int32_t kind)
{
    int32_t y = a->f2d4, x = a->f2d0;
    uint8_t *q = g_world->f1044;
    const wl_room *room = g_world->room;
    if (room == NULL) return;
    if (kind != 2 && (g_world->f1034 != 0 || g_world->scene_kind == 2 || g_world->f2768 != 0 || room->f4080 != 0 ||
                      room->f40d0 == 0))
        return;
    put32(q, 0x90c, kind);
    put32(q, 0x10, 1);
    put32(q, 0x900, x);
    put32(q, 0x904, y);
    put32(q, 0x908, 1);
}

void party_gather(const wl_body *at)
{
    int32_t x = at->x, bottom = at->y + at->h;
    const wl_room *room = g_world->room;
    for (int k = 0; k < 8; k++) {
        const wl_pslot *s = room->slot[k];
        if (s->active != 1) continue;
        wl_char *c = s->ch;
        if (c == NULL || s == room->leader || c->base.alive != 1) continue;
        wl_body *b = c->base.body;
        if (b->active != 1) continue;
        b->x = x;
        b->vel_x = 0;
        b->y = bottom - b->h;
        b->vvel = 0;
        b->c_flag = grid_c_query(g_world->grid, b->w / 2 + x, b->y + b->h - 1, NULL);
        b->slow = grid_c_query(g_world->grid, b->w / 2 + b->x, b->margin + b->y, NULL);
        b->state = 0; b->sub = 0; b->ctr = 0;
        b->pose = 0;
        b->act_len = 210;
        b->f58 = 0;
        b->act_ctr = 0;
        b->act_78 = 0;
        b->act_arg = 0;
        b->spell = 0;
        b->target = 0;
        b->act_5a = 0;
        b->act_64 = 0;
        char_ai_reset(c, 0);
        c->invuln_b = 40;
        c->invuln_a = 40;
    }
}

/* ── switching ─────────────────────────────────────────────────────────────── */

enum { BY_STEP, BY_SET, BY_LINK };   /* the inliner: 0x59b170 / 0x40afe0 / 0x40be30 */

/* 0x40afe0's inline of the 70220 shot emitters (the slot 0x40ac90 elsewhere) */
static void shots_inline(wl_actor *a, wl_body *b, int32_t code, uint32_t serial, int32_t a15, int32_t a16, int32_t a17)
{
    int32_t k = 3;
    sfx_play_at(&a->base, 0, 235, 0, 0, 0);
    for (int n = 2; n != 0; n--) {
        for (uint16_t m = 10; m < 50; m += 10)
            g_eng.h55cfc0(a, 0, 2, code, IP(b), 2000, 0, 1, 0, m, 0, 90, (intptr_t)serial, k, a15, a16, a17, 0, 0, 2);
        k = 1;
    }
}

/* the per-code effect of switching `a` (state `st`) to `v`; 0 = the 70230 hold (a resting
 * crate plate that is on stays on: nothing changes) */
static int switch_effect(wl_actor *a, wl_body *b, int32_t v, int32_t st, int by)
{
    int32_t code = a->base.code;
    if (code > 70400) {
        if (code == 70450) {
            if (v != 1) return 1;
            int32_t f = -2000 - mul(rnd(), 2000) / 0x8000;
            if (by == BY_SET) sfx_oneshot(&a->base, sound_pool(241), sfx_volume(), 0, f);
            else sfx_play_at(&a->base, 0, 241, 0, f, 0);
            entity_dust(&a->base, b, 0, 20, 0, 0);
        } else if (code == 70820) {
            if (v == 0) entity_set_collide(&a->base, 3, 3);
            else if (v == 1) entity_set_collide(&a->base, 0, 0);
        } else if (code == 70850) {
            if (v == 1) party_gather(b);
        }
        return 1;
    }
    if (code == 70400) {
        if (v != 1) return 1;
        int32_t f = 1000 - mul(rnd(), 3000) / 0x8000;
        if (by == BY_STEP) sfx_play_at(&a->base, 0, 242, 0, f, 0);
        else sfx_oneshot(&a->base, sound_pool(242), sfx_volume(), 0, f);
        entity_dust(&a->base, b, 0, 20, 0, 0);
        a->f284 = 1;
        actor_unlist_all(a);
        a->material = 0;
        return 1;
    }
    switch (code) {
    case 70220:
        if (v != 1) break;
        switch (a->step) {
        case 90:
            if (by == BY_SET) shots_inline(a, b, 100019, world_next_serial(g_world), 3, 800, 0);
            else g_eng.h40ac90(a, IP(b), 0, 100019, 3, 800, 0);
            break;
        case 91: a->invert = 1; break;
        case 92: warp_request(a, 2); break;
        case 93: actor_shot(a, b, 0, 1200); b->pose = 147; break;
        case 94:
            if (by == BY_SET) {
                int32_t pw = a->f2e0[1];
                shots_inline(a, b, 100041, world_next_serial(g_world), 7, 1000, pw);
            } else g_eng.h40ac90(a, IP(b), 0, 100041, 7, 1000, a->f2e0[1]);
            break;
        default: break;
        }
        break;
    case 70230:
        if (a->kind == 1 && st == 1) return 0;
        break;
    case 70251: case 70252: case 70371:
        switch_glow(a, v, 1);
        break;
    default: break;
    }
    return 1;
}

/* the new state's step record: its clip on body `b` (0x407b80) and its sound */
static int apply_step(wl_actor *a, wl_body *b, int32_t v)
{
    uint16_t n = a->nsteps;
    a->base.f00c = v;
    for (uint16_t i = 0; i < n; i++) {
        if (a->steps[i].state != v) continue;
        if (a->steps[i].clip != NULL) body_set_clip(b, a->steps[i].clip);
        if (a->steps[i].sfx != 0) sfx_play_at(&a->base, 0, a->steps[i].sfx, 0, 0, 0);
        return 1;
    }
    return 0;
}

static int run_script(wl_actor *a, int32_t mode)
{
    return event_run(g_world->event, g_world->f1044, g_world->room, a, 0, 0, mode);
}

int actor_switch_off(wl_actor *a)
{
    int32_t on = a->base.f00c;
    wl_body *b = a->base.body;
    if (on == 0) return 0;
    int32_t code = a->base.code;
    if (code > 70400) {
        if (code == 70820) entity_set_collide(&a->base, 3, 3);
    } else if (code != 70400) {
        switch (code) {
        case 70230: if (a->kind == 1 && on == 1) return 0; break;
        case 70251: case 70252: case 70371: switch_glow(a, 0, 1); break;
        default: break;
        }
    }
    apply_step(a, b, 0);
    return 1;
}

int switch_set(wl_actor *a, int32_t v, int32_t quiet)
{
    int32_t ret = 0;
    wl_body *b = a->base.body;
    if (v == a->base.f00c) return 0;
    if (!switch_effect(a, b, v, a->base.f00c, BY_SET)) return 0;
    apply_step(a, b, v);
    if (quiet != 0) return 0;
    if (a->base.handle != 0) {
        int r = run_script(a, 3);
        if (r == 6) return 6;
        if (r != 0) ret = r;
    }
    int r = switch_link(a, v, 0);
    if (r == 0) return ret;
    return r;
}

int switch_link(wl_actor *a, int32_t v, int32_t quiet)
{
    int32_t ret = 0;
    if (a->link == 0) return 0;
    if (a->invert != 0) v = v == 0;
    for (uint16_t i = 0; i < g_world->list_2790->count; i++) {
        wl_actor *e = (wl_actor *)g_world->list_2790->items[i];
        if (a->link != e->base.handle) continue;
        int32_t st = e->base.f00c;
        wl_body *b = e->base.body;
        if (v != st && switch_effect(e, b, v, st, BY_LINK)) {
            int32_t got = 0;
            apply_step(e, b, v);
            if (quiet == 0) {
                if (e->base.handle != 0) {
                    int r = run_script(e, 3);
                    if (r == 6) return 6;
                    if (r != 0) got = r;
                }
                int r = switch_link(e, v, 0);
                if (r == 6) return 6;
                if (r != 0) got = r;
            }
            if (got != 0) ret = got;
        }
        if (a->link_all == 0) return ret;
    }
    return ret;
}

/* ── the pass ──────────────────────────────────────────────────────────────── */

/* the pending hit (+0x294 by +0x290), then the hit script (mode 4) */
static int step_tail(wl_actor *a, int32_t ret)
{
    if (a->hit == 0) return ret;
    int32_t m = a->material;
    int32_t r = ret;
    a->hit = 0;
    switch (m) {
    case 1: case 2:
        actor_break(a, a->base.body);
        if (a->step != 0 && a->base.f00c == 0) {
            a->f284 = 1;
            actor_unlist_all(a);
            r = switch_set(a, 1, 0);
            a->material = 0;
        } else actor_release(a);
        break;
    case 3: r = switch_set(a, a->base.f00c == 1 ? 0 : 1, 0); break;
    case 4: if (a->base.f00c == 0) r = switch_set(a, 1, 0); break;
    default: break;
    }
    if (a->base.handle == 0) return r;
    int e = run_script(a, 4);
    if (e == 0) return r;
    return e;
}

/* 70830: the timed plate.  On: characters of the id range in the widened area start a
 * two-tick countdown (+0x58 / +0x5c of the body) that ends in the switch-on, unless the
 * strip above the plate is empty; off again the same way once they left. */
static int32_t timed_plate(wl_actor *a, wl_body *b)
{
    int32_t ret = 0;
    int32_t r[4] = { a->area_x0 - 3200, a->area_y0, a->area_x1 - a->area_x0 + 9600, a->area_y1 - a->area_y0 };
    int on = chars_in_rect_count(0, r, (uint32_t)a->step, (uint32_t)a->f2d0) != 0;
    int32_t st = a->base.f00c;
    if (st == 0) {
        if (!on) {
            b->f58 = 0;
            switch_link(a, a->base.f00c, 0);
            return 0;
        }
        if (b->f58 == 0) {
            int32_t r2[4] = { b->x, a->area_y0, 6400, a->area_y1 - a->area_y0 };
            if (char_in_rect(0, r2, 0) != 0) { b->act_ctr = 2; b->f58 = 1; }
        } else if (b->f58 == 1) {
            if (b->act_ctr > 0) b->act_ctr--;
            else { b->f58 = 0; ret = switch_set(a, 1, 0); }
        }
    } else if (st == 1) {
        if (b->f58 == 0) {
            if (!on || char_in_rect(0, r, 0) == 0) { b->act_ctr = 2; b->f58 = 1; }
        } else if (b->f58 == 1) {
            if (b->act_ctr > 0) b->act_ctr--;
            else { b->f58 = 0; ret = switch_set(a, 0, 0); }
        }
    }
    switch_link(a, a->base.f00c, 0);
    return ret;
}

int gimmick_step(wl_actor *a, wl_char *pl)
{
    int32_t ret = 0;
    wl_body *b = a->base.body;
    if (a->hold != 0) {
        a->hold--;
        return 0;
    }
    int32_t code = a->base.code;
    switch (code) {
    case 70215: {   /* the weight plate */
        int32_t kind = 0, heavy = 1, reach = 0;
        if (a->kind == 2) { kind = 2; heavy = 0; reach = 400; }
        if (entity_count_riders(&a->base, kind, heavy, 0, NULL, reach) > 0) ret = switch_set(a, 1, 0);
        else if (a->kind <= 0 || a->kind > 2) ret = switch_set(a, 0, 0);
        break;
    }
    case 70130:     /* the plate the player stands on */
        if (a->base.f00c == 0) {
            if (overlaps(a, pl)) ret = switch_set(a, 1, 0);
        } else if (a->base.f00c == 1) {
            if (!overlaps(a, pl)) actor_switch_off(a);
        }
        break;
    case 70100:     /* the one-shot plate */
        if (a->base.f00c == 0 && overlaps(a, pl)) {
            sfx_play(g_eng.sys, 12, 0, 0);
            a->f284 = 1;
            ret = switch_set(a, 1, 0);
        }
        break;
    case 70600:     /* the lever a rider pulls off */
        if (a->kind != 2 || char_riding(a, 1, NULL) == NULL) break;
        if (!actor_switch_off(a)) break;
        if (a->base.handle == 0 || run_script(a, 3) != 6) switch_link(a, 0, 0);
        break;
    case 70800: {   /* the area trigger */
        int32_t r[4] = { a->area_x0, a->area_y0, a->area_x1 - a->area_x0, a->area_y1 - a->area_y0 };
        int hit;
        if (a->f2d0 > 0) {
            wl_entity *o = actor_by_handle(a->f2d0);
            hit = o != NULL && entity_rect_hit(o, r, 0);
        } else hit = char_in_rect(0, r, 0);
        if (hit) ret = switch_set(a, 1, 0);
        else if (a->kind != 1) ret = switch_set(a, 0, 0);
        break;
    }
    case 70810: {   /* the follower */
        int32_t v = 0;
        if (a->link == 0) v = a->base.f00c;
        else {
            const wl_entity *o = actor_by_handle(a->link);
            if (o != NULL) v = o->f00c == 0 ? a->step != 1 : a->step == 1;
        }
        int32_t st = a->base.f00c;
        wl_body *bb = a->base.body;
        if (v != st && switch_effect(a, bb, v, st, BY_STEP)) apply_step(a, bb, v);
        a->hold = 20;
        break;
    }
    case 70830:
        ret = timed_plate(a, b);
        break;
    default: break;
    }
    return step_tail(a, ret);
}

int gimmick_pass(wl_roomloop *rl, wl_char *player)
{
    for (uint16_t i = 0; i < 0x80; i++) {
        wl_actor *a = (wl_actor *)g_world->actors[i];
        if (a->base.alive == 0) continue;
        int r = gimmick_step(a, player);
        if (r == 0) continue;
        if (r == 1 || r == 2 || r == 6) return r;
        /* a script ran: drop what is held */
        void *kb = sysp(0x2878);
        g_eng.h5ba4c0(kb);
        g_eng.h5ba2d0(sysp(0x2878));
        for (uint32_t j = 0; j < 2; j++) {
            void *joy = sysp(0x287c + 4 * j);
            if (joy == NULL) continue;
            g_eng.h5ba650(joy);
            g_eng.h49bf90(joy, mul((**g_eng.keycfg)->joy_f53c, 1000) / 1000);
        }
        for (uint16_t j = 0; j < rl->nrooms; j++)
            for (int k = 0; k < 8; k++) {
                const wl_pslot *s = rl->rooms[j].slot[k];
                if (s->active == 1 && s->id != 100000104 && s->in != NULL) input_mgr_reset(s->in);
            }
    }
    return 0;
}
