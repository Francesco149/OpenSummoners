/* actor_ops.c — per-code actor behaviours.  See actor_ops.h.
 *
 * Transcribed from the asm.  0x5564e0 sums the riders' u16 weights in a register whose
 * upper half holds stale bits (the weight pointer's, a dword read of the recursion's u16
 * result); only the low 16 bits are stored, so the sum here is formed in u16. */
#include "actor_ops.h"

#include <stddef.h>
#include <string.h>

#include "char_integrate.h"
#include "engine_calls.h"
#include "particle_spawn.h"
#include "entity_life.h"
#include "entity_move.h"
#include "gimmick.h"
#include "rng.h"
#include "sound_fx.h"

static int32_t iabs(int32_t v) { return v < 0 ? (int32_t)(0u - (uint32_t)v) : v; }
static int32_t mul(int32_t a, int32_t b) { return (int32_t)((uint32_t)a * (uint32_t)b); }
static int32_t rnd(void) { return (int32_t)rng_rand(); }
static int32_t sfx_volume(void) { return (**g_eng.keycfg)->sfx_volume; }

/* the 100080 dust particle at `dx` along / `dy` below body `b` */
static void dust(wl_entity *e, wl_body *b, int32_t dx, int32_t dy)
{
    particle_spawn(e, e, 100080, b, dx, dy, 99, 4, 0, 0, 2, 0, 0, 10);
}

void entity_dust(wl_entity *e, wl_body *b, int32_t a3, uint32_t n, int32_t dx, int32_t dy)
{
    if (a3 != 0 || (uint16_t)n == 0) return;
    for (uint32_t i = (uint16_t)n; i != 0; i--) {
        int32_t span = b->w + 1600;
        dust(e, b, mul(rnd(), span) / 0x8000 - span / 2 + dx, dy);
    }
}

void actor_break(wl_actor *a, wl_body *b)
{
    if (a->material == 1) {
        int32_t freq = 1000 - mul(rnd(), 3000) / 0x8000;
        sfx_oneshot(&a->base, sound_pool(242), sfx_volume(), 0, freq);
    } else if (a->material == 2) {
        int32_t freq = -2000 - mul(rnd(), 2000) / 0x8000;
        sfx_oneshot(&a->base, sound_pool(241), sfx_volume(), 0, freq);
    }
    for (int i = 20; i != 0; i--) {
        int32_t span = b->w + 1600;
        dust(&a->base, b, mul(rnd(), span) / 0x8000 - span / 2, 0);
    }
}

int actor_slide_v(wl_actor *a, wl_body *b, int32_t speed)
{
    if (b->vdir == 2) {
        if (b->y >= a->area_y1) {
            b->vdir = 0;
            return 1;
        }
        int32_t y = b->y + speed;
        b->y = a->area_y1 < y ? a->area_y1 : y;
        return 0;
    }
    if (b->y <= a->area_y0) {
        b->vdir = 2;
        return 1;
    }
    int32_t y = b->y - speed;
    b->y = a->area_y0 > y ? a->area_y0 : y;
    return 0;
}

int entity_count_riders(wl_entity *e, int32_t kind, int32_t heavy, int32_t fac, uint16_t *weight, int32_t reach)
{
    uint16_t w = 0;
    int n = 0;
    for (uint16_t k = 0; k < g_world->list_2794->count; k++) {
        wl_entity *r = g_world->list_2794->items[k];
        if (r->body->attach != e) continue;
        if (kind != 0 && kind != r->rider_kind) continue;
        if (fac != 0 && fac != r->faction) continue;
        if (reach != 0 && reach <= iabs(e->body->x + (e->body->w / 2 - r->body->w / 2 - r->body->x))) continue;
        if (heavy != 0 && r->rider_heavy == 0) continue;
        if (weight != NULL) {
            uint16_t sub;
            w = (uint16_t)(w + r->weight);
            entity_count_riders(r, kind, heavy, fac, &sub, reach);
            w = (uint16_t)(w + sub);
        }
        n++;
    }
    if (weight != NULL) *weight = w;
    return n;
}

/* the landing: break (released), or a thud with dust */
static int crate_land(wl_actor *a, wl_body *b, int32_t probe, int32_t break_v)
{
    if (probe != 0) return 0;
    if (break_v > 0 && b->vvel >= break_v) {
        actor_break(a, b);
        actor_release(a);
        return 1;
    }
    if (b->vvel >= 20000) {
        entity_dust(&a->base, b, 0, 20, 0, 0);
        sfx_play_at(&a->base, 0, 37, 2000, 0, 0);
    }
    return 0;
}

/* one tick of gravity; 1 = landed */
static int crate_fall(wl_actor *a, wl_body *b, uint16_t part)
{
    int32_t v = b->vvel + 2000;
    if (v > 80000) v = 80000;
    b->vvel = v;
    return entity_move_vertical(&a->base, b, v / 100, 0, part, &b->y, NULL) == 0;
}

int actor_crate(wl_actor *a, wl_body *b, int32_t probe, uint16_t part, int32_t x0, int32_t y0, int32_t hold,
                int32_t pushed, int32_t speed, int32_t break_v, int32_t slide, int32_t slide_sfx, uint16_t push_sfx)
{
    int moved = 0;
    switch (a->kind) {
    case 1:
        switch (b->f58) {
        case 0:
            if (entity_count_riders(&a->base, 0, 1, 0, NULL, 0) > 0) {
                b->f58++;
                if (probe == 0) sfx_oneshot(&a->base, sound_pool(38), sfx_volume(), 0, 0);
            }
            goto slide;
        case 1:
            if (crate_fall(a, b, part)) {
                crate_land(a, b, probe, break_v);
                b->f58++;
                b->vvel = 0;
            }
            goto slide;
        case 2:
            if (pushed != 0) {
                a->base.f1c4 = 100;
                a->use_mover = 1;
                a->kind = 2;
            }
            body_action_reset(b);
            goto done;
        default:
            goto slide;
        }
    case 2:
        if (probe == 0) {
            if (a->base.f03c != 0) {
                if (a->cooldown == 0) a->cooldown = 60;
                if (a->cooldown == 50) sfx_oneshot(&a->base, sound_pool(push_sfx), sfx_volume(), 0, 0);
                else a->base.f03c = 0;
            } else {
                a->cooldown = 0;
            }
        }
        break;
    case 4:
        break;
    default:
        goto slide;
    }

    /* kinds 2 / 4: the fall */
    if (b->f58 == 0) {
        if (hold != 0) {
            b->vvel = 0;
            goto done;
        }
        b->f58 = 1;
    } else if (b->f58 != 1) {
        goto done;
    }
    b->vel_x = 0;
    if (crate_fall(a, b, part)) {
        crate_land(a, b, probe, break_v);
        b->vvel = 0;
        b->f58 = 0;
    }
    goto done;

slide:
    if (slide == 0) goto done;
    if (a->area_x0 != a->area_x1) {
        int turn = 0;
        if (b->facing == 1) {
            if (b->x < a->area_x1) {
                int32_t x = b->x + speed;
                b->x = a->area_x1 < x ? a->area_x1 : x;
            } else {
                b->facing = 3;
                turn = 1;
            }
        } else if (b->x > a->area_x0) {
            int32_t x = b->x - speed;
            b->x = a->area_x0 > x ? a->area_x0 : x;
        } else {
            b->facing = 1;
            turn = 1;
        }
        if (turn && slide_sfx != 0) sfx_play_at(&a->base, probe, 221, -1000, 0, 0);
        moved = 1;
    }
    if (a->area_y0 != a->area_y1) {
        if (actor_slide_v(a, b, speed) != 0 && slide_sfx != 0) sfx_play_at(&a->base, probe, 221, -1000, 0, 0);
        moved = 1;
    }

done:
    entity_carry_riders(&a->base, b->x - x0, b->y - y0, part);
    return moved;
}

void actor_state_restore(wl_actor *a, int32_t st)
{
    wl_body *b = a->base.body;
    a->base.f00c = st;
    for (uint16_t k = 0; k < a->nsteps; k++) {
        if (a->steps[k].state != st || a->steps[k].clip == NULL) continue;
        if (b->clip != a->steps[k].clip) {
            b->clip = a->steps[k].clip;
            b->clip_done = 0;
            b->clip_frame = 0;
            b->clip_timer = 0;
        }
        uint16_t nf;
        memcpy(&nf, (const uint8_t *)a->steps[k].clip + 0x42, 2);   /* the clip's frame count */
        b->clip_frame = (uint16_t)(nf - 1);
    }
    uint32_t code = (uint32_t)a->base.code;
    int32_t s = a->base.f00c;
    if (code == 70251 || code == 70252 || code == 70371) {
        int32_t p = a->f2e4 == 1 ? 100135 : 100134;
        const wl_body *pb = a->base.body;
        if (s == 0) {
            if (particle_find(&a->base, 0) == NULL)
                particle_spawn(&a->base, &a->base, p, pb, 0, 0, 99, 1, 0, 0, 2, 1, 0, 20);
        } else if (s == 1) {
            if (particle_find(&a->base, 0) != NULL) g_eng.h556e50((intptr_t)a, 0, 0);
        }
        return;
    }
    switch (code) {
    case 70100:
        a->f284 = s == 1;
        return;
    case 70120:
        if (s == 1) {
            b->y += b->h - a->grow_min;
            b->h = a->grow_min;
            int32_t one = 1;
            memcpy(g_world->grid + 0x28, &one, 4);   /* the grid refresh flag (0x58ff50) */
        } else {
            b->y += b->h - a->grow_max;
            b->h = a->grow_max;
        }
        return;
    case 70230:
        if (s == 0) b->h = 12800;
        else if (s == 1) b->h = 3200;
        b->dst_y = (b->h - a->base.height) / 100;
        return;
    case 70281:
    case 70290:
        if (s == 1) {
            a->f284 = 1;
            entity_set_collide(&a->base, 0, 0);
        } else {
            a->f284 = 0;
            entity_set_collide(&a->base, 3, 2);
        }
        return;
    case 70400:
        if (s == 1) {
            a->f284 = 1;
            wl_list *l = g_world->list_2798;
            uint16_t n = l->count, at = n;
            for (uint16_t i = 0; i < n; i++)
                if (l->items[i] == &a->base) {
                    at = i;
                    break;
                }
            if (at < n) {
                for (uint16_t i = at; (int32_t)i < (int32_t)l->count - 1; i++) l->items[i] = l->items[i + 1];
                l->count--;
            }
            entity_unlist(&a->base);
            a->material = 0;
        }
        return;
    case 70440:
        if (s == 0) b->y = a->area_y0;
        else if (s == 1) b->y = a->area_y0 + 19200;
        return;
    case 70610:
    case 70620:
    case 70810:
        if (s == 0) {
            a->f284 = 0;
            b->act_ctr = 1000;
        } else if (s == 1) {
            a->f284 = 1;
            b->act_ctr = 0;
        }
        return;
    case 70820:
        if (st == 0) entity_set_collide(&a->base, 3, 3);
        else if (st == 1) entity_set_collide(&a->base, 0, 0);
        return;
    default:
        return;
    }
}
