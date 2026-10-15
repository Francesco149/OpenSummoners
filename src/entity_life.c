/* entity_life.c — entity list membership + release.  See entity_life.h.
 *
 * Transcribed from the asm (0x556180 is thiscall with one stack argument although IDA
 * types it fastcall; 0x555fc0 takes only ecx).  The list removal is retail's: find the
 * first slot holding the entity (u16 index), shift the rest down, count - 1. */
#include "entity_life.h"

#include <stddef.h>

#include "engine_calls.h"
#include "entity_fx.h"
#include "entity_move.h"
#include "sound_fx.h"
#include "char_def.h"   /* voice_set_stop 0x41e440 */

/* the multi-part table at entity+0x8 (0x445e50 builds it) */
typedef struct parts_tbl {
    void    *a;          /* +0x00 */
    void    *b;          /* +0x04 */
    uint16_t n;          /* +0x08 */
    uint8_t  _p0a[0x14 - 0xa];
    int32_t  f14, f18, f1c;
} parts_tbl;

void wl_list_remove(wl_list *l, const wl_entity *e)
{
    uint16_t n = l->count, i = n;
    for (uint16_t k = 0; k < n; k++)
        if (l->items[k] == e) { i = k; break; }
    if (i >= n) return;
    while ((int32_t)i < (int32_t)l->count - 1) {
        l->items[i] = l->items[i + 1];
        i++;
    }
    l->count--;
}

void entity_unlist(wl_entity *e)
{
    wl_list_remove(g_world->collide, e);
    wl_list_remove(g_world->list_2788, e);
    wl_list_remove(g_world->list_2790, e);
    wl_list_remove(g_world->list_2794, e);
}

void entity_release(wl_entity *e, int32_t listed)
{
    if (listed == 0) entity_unlist(e);
    if (e->rects != NULL) {
        g_eng.op_delete(e->rects);
        e->rects = NULL;
        for (uint16_t i = 0; i < e->nbody; i++)
            if (e->body[i].frames != NULL) {
                g_eng.op_delete(e->body[i].frames);
                e->body[i].frames = NULL;
            }
    }
    parts_tbl *t = e->f08;
    if (t != NULL) {
        t->f14 = 0;
        t->f18 = 0;
        t->f1c = 0;
        if (t->a != NULL) { g_eng.op_delete(t->a); t->a = NULL; }
        if (t->b != NULL) { g_eng.op_delete(t->b); t->b = NULL; }
        t->n = 0;
        g_eng.op_delete(t);
        e->f08 = NULL;
    }
    if (e->snd != NULL) {
        for (uint16_t i = 0; i < e->nsnd; i++) entity_snd_release(e, i, 0, 1);
        if (e->snd != NULL) {
            g_eng.op_delete(e->snd);
            e->snd = NULL;
        }
    }
    e->alive = 0;
}

void actor_release(wl_actor *a)
{
    if (a->f1f8 != NULL) {
        actor_release(a->f1f8);
        a->f1f8 = NULL;
    }
    wl_list_remove(g_world->list_2798, &a->base);
    entity_unlist(&a->base);
    entity_release(&a->base, 1);
    if (a->f2a8 != NULL) {
        g_eng.op_delete(a->f2a8);
        a->f2a8 = NULL;
    }
}

void char_release(wl_char *c)
{
    wl_char *k = c->carrier;
    if (k != NULL && k->can_carry != 0 && k->held != NULL) {
        k->held->carrier = NULL;
        k->held = NULL;
    }
    if (c->held != NULL && c->can_carry != 0) {
        c->held->carrier = NULL;
        c->held = NULL;
    }
    for (int j = 0; j < 2; j++)
        if (c->fc824[j] != 0) {
            entity_release((wl_entity *)(intptr_t)c->fc824[j], 1);
            c->fc824[j] = 0;
        }
    for (int j = 0; j < 10; j++)
        if (c->linked[j] != NULL) {
            entity_release(c->linked[j], 0);
            c->linked[j] = NULL;
        }
    entity_release(&c->base, 0);
    wl_voiceset *vs = c->voice;
    if (vs != NULL) {
        if (vs->grp != NULL) {
            for (uint16_t i = 0; i < vs->n; i++)
                for (uint16_t k2 = 0; k2 < vs->grp[i].n; k2++) vline_stop(&vs->grp[i].line[k2], 1);
            g_eng.op_delete(vs->grp);
            vs->grp = NULL;
        }
        vs->n = 0;
        g_eng.op_delete(vs);
        c->voice = NULL;
    }
}

void char_unload(wl_char *c)
{
    if (c == NULL) return;
    /* as 0x4467d0, but the voice lines stop through 0x41e440 */
    wl_char *k = c->carrier;
    if (k != NULL && k->can_carry != 0 && k->held != NULL) {
        k->held->carrier = NULL;
        k->held = NULL;
    }
    if (c->held != NULL && c->can_carry != 0) {
        c->held->carrier = NULL;
        c->held = NULL;
    }
    for (int j = 0; j < 2; j++)
        if (c->fc824[j] != 0) {
            entity_release((wl_entity *)(intptr_t)c->fc824[j], 1);
            c->fc824[j] = 0;
        }
    for (int j = 0; j < 10; j++)
        if (c->linked[j] != NULL) {
            entity_release(c->linked[j], 0);
            c->linked[j] = NULL;
        }
    entity_release(&c->base, 0);
    wl_voiceset *vs = c->voice;
    if (vs != NULL) {
        if (vs->grp != NULL) {
            voice_set_stop(vs);
            g_eng.op_delete(vs->grp);
            vs->grp = NULL;
        }
        vs->n = 0;
        g_eng.op_delete(vs);
        c->voice = NULL;
    }
}

void entity_set_collide(wl_entity *e, int32_t mode, int32_t kind)
{
    e->coll_mode = mode;
    e->coll_kind = kind;
    if (mode == 0 || kind <= 0 || kind > 3) return;
    wl_list *l = g_world->collide;
    if (l->count < l->cap) l->items[l->count++] = e;
}

void entity_carry_riders(wl_entity *e, int32_t dx, int32_t dy, uint16_t part)
{
    if (dx == 0 && dy == 0) return;
    for (uint16_t k = 0; k < g_world->list_2794->count; k++) {
        wl_entity *r = g_world->list_2794->items[k];
        if (r->nbody <= part) continue;
        wl_body *rb = &r->body[part];
        if (rb->attach != e) continue;
        int32_t x0 = rb->x, y0 = rb->y;
        entity_move_horizontal(r, rb, dx, 0, 0, rb->drop, part, 0);
        rb = &r->body[part];
        entity_move_vertical(r, rb, dy, rb->drop, part, &rb->y, NULL);
        entity_carry_riders(r, r->body[part].x - x0, r->body[part].y - y0, part);
    }
}

void entity_set_handle(wl_entity *e, int32_t h)
{
    if (e->handle != 0) wl_list_remove(g_world->list_2790, e);   /* (the same loop, inlined) */
    if (h != 0) {
        wl_list *l = g_world->list_2790;
        if (l->count < l->cap) l->items[l->count++] = e;
    }
    e->handle = h;
}
