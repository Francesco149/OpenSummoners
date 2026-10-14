/* entity_move.c — FUN_0054db10 / FUN_0054e5c0 / FUN_004416c0.  See entity_move.h.
 *
 * The decompiled order of the per-entity gates is kept except one place: retail reads
 * the other entity's body[part].active / .nocollide BEFORE checking part < nbody (an
 * out-of-range read when part >= nbody); the port tests part < nbody first — the
 * three conditions are pure reads OR'ed into one skip, so the result is identical. */
#include "entity_move.h"

#include <stddef.h>

wl_world     *g_world;
coll_slope_fn g_world_slope;
void         *g_world_slope_ctx;

int faction_relation(int32_t a, int32_t b)
{
    if (a == 0 || b == 0) return 0;
    if (a == 3 || b == 3) return 1;
    return (a == b) + 1;
}

/* The part of `e` the movers compare against: `part` when e has it, else part 0. */
static const wl_body *part_of(const wl_entity *e, uint16_t part)
{
    return &e->body[part < e->nbody ? part : 0];
}

/* The shared mode gate (54db10 / 54e5c0): 1 = `e` can block `self`'s body. */
static int coll_gate(const wl_entity *self, const wl_body *body, const wl_entity *e, uint16_t part)
{
    if (part >= e->nbody || !e->body[part].active || e->body[part].nocollide) return 0;
    switch (e->coll_mode) {
    case 0: return 0;
    case 1: return body->nocollide == 0;
    case 2: return body->nocollide == 0 && e->coll_off == 0 && self->coll_off == 0 &&
                   faction_relation(e->faction, self->faction) == 1;
    default: return 1;
    }
}

int entity_move_horizontal(wl_entity *self, wl_body *body, int32_t delta,
                           int32_t step_down, int32_t step_up, int16_t pass,
                           uint16_t part, int32_t probe)
{
    int32_t top = body->y + body->margin, bot = body->h + body->y - 1;
    int32_t left = body->x, right = body->x + body->w;
    if (delta == 0) return 1;

    const wl_list *L = g_world->collide;
    if (self->no_entity_coll == 0) {
        for (uint16_t i = 0; i < L->count; i++) {
            const wl_entity *e = L->items[i];
            if (e == self) continue;
            const wl_body *eb = part_of(e, part);
            if (!(top < eb->h + eb->y - 1 && bot > eb->y + eb->margin)) continue;   /* no Y overlap */
            if (!coll_gate(self, body, e, part)) continue;
            if (e->coll_kind != 1 && e->coll_kind != 3) continue;
            if (e->body[0].attach == self || self->body[0].attach == e) continue;
            int32_t ex0 = eb->x, ex1 = eb->x + eb->w;
            if (left >= ex0 && left <= ex1 && right >= ex0 && right <= ex1) continue;   /* already inside */
            if (delta <= 0) {
                if (left >= ex0 && delta + left <= ex1) return 0;
            } else {
                if (delta + right >= ex0 && right <= ex1) return 0;
            }
        }
    }

    int32_t sink_x = 0, sink_y = 0;          /* probe: retail aims at its own arg slot + a local */
    int32_t *ox = probe ? &sink_x : &body->x, *oy = probe ? &sink_y : &body->y;
    return collision_sweep_horizontal(g_world->grid, body->x, body->w, top, bot, delta,
                                      step_down, step_up, pass != 0, g_world_slope,
                                      g_world_slope_ctx, ox, oy);
}

int entity_move_vertical(wl_entity *self, wl_body *body, int32_t delta,
                         int32_t pass, uint16_t part, int32_t *out_y,
                         wl_entity **support)
{
    int32_t top = body->y + body->margin, bot = body->h + body->y - 1;
    int32_t left = body->x, right = body->x + body->w;
    int32_t d = delta;
    int ok = 1;
    wl_entity *sup = NULL;
    if (delta == 0) return 1;

    const wl_list *L = g_world->collide;
    if (self->no_entity_coll == 0) {
        for (uint16_t i = 0; i < L->count; i++) {
            wl_entity *e = L->items[i];
            if (e == self) continue;
            const wl_body *eb = part_of(e, part);
            if (!(left < eb->x + eb->w && right > eb->x)) continue;                  /* no X overlap */
            if (!coll_gate(self, body, e, part)) continue;
            if (delta <= 0) {
                if (e->coll_kind != 3) continue;
                if (top >= eb->y + eb->margin && delta + top <= eb->h + eb->y - 1) return 0;
            } else {
                if (e->coll_kind == 2) { if ((int16_t)pass != 0) continue; }
                else if (e->coll_kind != 3) continue;
                int32_t etop = eb->y + eb->margin;
                if (delta + bot >= etop && delta + bot <= eb->h + eb->y - 1) {
                    int32_t reach = etop - bot - 1;
                    if (reach < d && reach > -3200) { ok = 0; d = reach; sup = e; }
                }
            }
        }
    }

    phys_box b = { body->x, body->y, body->w, body->h, 0 };
    if (collision_move_vertical(g_world->grid, &b, d, (int16_t)pass != 0, g_world_slope,
                                g_world_slope_ctx, out_y) == 0)
        ok = 0;
    if (support != NULL) *support = sup;
    return ok;
}
