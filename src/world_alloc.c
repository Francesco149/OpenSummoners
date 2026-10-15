/* world_alloc.c — FUN_004017d0 / FUN_0054bfb0 + the entity frees.  See world_alloc.h.
 *
 * Transcribed from the asm; every block comes from the engine's allocator in retail's order. */
#include "world_alloc.h"

#include <stddef.h>
#include <string.h>

#include "engine_calls.h"
#include "room_load.h"

static void st32(void *p, uint32_t off, int32_t v) { memcpy((uint8_t *)p + off, &v, 4); }
static void st16(void *p, uint32_t off, uint16_t v) { memcpy((uint8_t *)p + off, &v, 2); }
static void st8(void *p, uint32_t off, uint8_t v) { ((uint8_t *)p)[off] = v; }
static void *ldp(const void *p, uint32_t off) { void *v; memcpy(&v, (const uint8_t *)p + off, sizeof v); return v; }
static void stp(void *p, uint32_t off, const void *v) { memcpy((uint8_t *)p + off, &v, sizeof v); }

/* every entity: no bodies, sound channels, rects, part table */
static void ent_init(wl_entity *e)
{
    e->body = NULL;
    e->snd = NULL;
    e->rects = NULL;
    e->f08 = NULL;
}

void char_block_init(uint8_t *b)
{
    st32(b, 0x00, 0);
    st32(b, 0x04, 0);
    st32(b, 0x10, 1000);
    st32(b, 0x14, 208);
    st32(b, 0x18, 208);
    st32(b, 0x0c, 0);
    st32(b, 0x08, 0);
    st32(b, 0x1c, 0);
    st16(b, 0x20, 0);
    st8(b, 0x2c, 0);
    st32(b, 0x24, 10);
    st32(b, 0x28, 0);
}

static wl_list *list_new(void)
{
    wl_list *l = g_eng.op_new(0xc);
    if (l != NULL) l->items = NULL;
    return l;
}
static void list_alloc(wl_list *l)
{
    if (l->items != NULL) {
        g_eng.op_delete(l->items);
        l->items = NULL;
    }
    l->cap = 128;
    l->items = g_eng.op_new(0x200);
    l->count = 0;
}

wl_world *world_new(void *mem)
{
    wl_world *w = mem;
    uint8_t *f = g_eng.op_new(0x2c);
    if (f != NULL) st32(f, 0, 0);
    w->f1040 = f;
    for (int i = 0; i < 64; i++) {
        wl_entity *e = g_eng.op_new(0x258);
        if (e != NULL) {
            ent_init(e);
            memset((uint8_t *)e + 0x1f8, 0, 0x60);
        }
        w->band64[i] = e;
    }
    for (int i = 0; i < 32; i++) {
        wl_char *c = g_eng.op_new(0x15bb0);
        if (c != NULL) {
            ent_init(&c->base);
            char_block_init((uint8_t *)c + 0x210);
            c->voice = NULL;
            memset((uint8_t *)c + 0x1422c, 0, 0x608);
            memset((uint8_t *)c + 0xc834, 0, 0x70);
            memset((uint8_t *)c + 0xc8a4, 0, 0x180);
            memset((uint8_t *)c + 0xca28, 0, 0x7800);
            memset((uint8_t *)c + 0x158d0, 0, 0x1d8);
            memset((uint8_t *)c + 0x14874, 0, 0x1030);
            memset((uint8_t *)c + 0x158a8, 0, 0x28);
            memset((uint8_t *)c + 0xc7a8, 0, 0x44);
        }
        w->chars[i] = c;
    }
    for (int i = 0; i < 128; i++) {
        wl_actor *a = g_eng.op_new(0x2fc);
        if (a != NULL) {
            ent_init(&a->base);
            a->f2a8 = NULL;
            a->f1f8 = NULL;
        }
        w->actors[i] = (wl_entity *)a;
    }
    for (int i = 0; i < 1024; i++) {
        wl_entity *e = g_eng.op_new(0x284);
        if (e != NULL) ent_init(e);
        w->particles[i] = e;
    }
    for (int i = 0; i < 96; i++) {
        wl_entity *e = g_eng.op_new(0x20c);
        if (e != NULL) ent_init(e);
        w->band96[i] = e;
    }
    for (int i = 0; i < 128; i++) {
        wl_entity *e = g_eng.op_new(0x1f8);
        if (e != NULL) ent_init(e);
        w->band2560[i] = e;
    }
    w->grid = g_eng.op_new(0x2cb044);
    void *v = g_eng.op_new(0x78);
    if (v != NULL) stp(v, 0x54, NULL);
    w->view = v;
    w->f1050 = g_eng.op_new(0xa52c);
    w->event = g_eng.op_new(0xb2c);
    w->list_2788 = list_new();
    w->collide = list_new();
    w->list_2790 = list_new();
    w->list_2794 = list_new();
    w->list_2798 = list_new();
    list_alloc(w->list_2788);
    list_alloc(w->collide);
    list_alloc(w->list_2790);
    list_alloc(w->list_2794);
    list_alloc(w->list_2798);
    w->room = NULL;
    w->action_serial = 0;
    w->f279c = 0;
    w->tick15 = 0;
    w->f2764 = 0;
    w->f2768 = 0;
    w->f2760 = 0;
    w->f277c = 0;
    st32(w, 0, 0);
    for (uint32_t k = 0; k < 8; k++) st32(w, 4 + 0x204 * k, 0);
    st32(w, 0x1024, 0);
    w->f1058 = NULL;
    st32(w, 0x1028, 0);
    st32(w, 0x102c, 0);
    return w;
}

/* the part table (entity +0x8): two buffers + a count */
static void parts_free(wl_entity *e)
{
    uint8_t *t = e->f08;
    if (t == NULL) return;
    void *b0 = ldp(t, 0);
    st32(t, 0x14, 0);
    st32(t, 0x18, 0);
    st32(t, 0x1c, 0);
    if (b0 != NULL) {
        g_eng.op_delete(b0);
        stp(t, 0, NULL);
    }
    if (ldp(t, 4) != NULL) {
        g_eng.op_delete(ldp(t, 4));
        stp(t, 4, NULL);
    }
    st16(t, 8, 0);
    g_eng.op_delete(t);
    e->f08 = NULL;
}

void entity_free(wl_entity *e)
{
    if (e->body != NULL) {
        g_eng.op_delete(e->body);
        e->body = NULL;
    }
    if (e->snd != NULL) {
        g_eng.op_delete(e->snd);
        e->snd = NULL;
    }
    if (e->rects != NULL) {
        g_eng.op_delete(e->rects);
        e->rects = NULL;
    }
    parts_free(e);
}

void actor_free(wl_actor *a)
{
    if (a->f2a8 != NULL) {
        g_eng.op_delete(a->f2a8);
        a->f2a8 = NULL;
    }
    entity_free(&a->base);
}

void char_free(wl_char *c)
{
    wl_voiceset *vs = c->voice;
    if (vs != NULL) {
        if (vs->grp != NULL) {
            for (uint16_t g = 0; g < vs->n; g++) {
                wl_vgroup *gr = &vs->grp[g];
                for (uint16_t l = 0; l < gr->n; l++) {
                    wl_voice *v = gr->line[l].voice;
                    if (v == NULL) continue;
                    g_eng.dsb_stop_5bb8e0(v->buf);
                    gr->line[l].voice->owner = 0;
                    gr->line[l].voice = NULL;
                }
            }
            g_eng.op_delete(vs->grp);
            vs->grp = NULL;
        }
        vs->n = 0;
        g_eng.op_delete(vs);
        c->voice = NULL;
    }
    entity_free(&c->base);
}

static void list_free(wl_list **lp)
{
    wl_list *l = *lp;
    if (l == NULL) return;
    if (l->items != NULL) {
        g_eng.op_delete(l->items);
        l->items = NULL;
    }
    g_eng.op_delete(l);
    *lp = NULL;
}

void world_free(wl_world *w)
{
    for (int i = 0; i < 64; i++) {
        wl_entity *e = w->band64[i];
        if (e == NULL) continue;
        entity_free(e);
        g_eng.op_delete(e);
        w->band64[i] = NULL;
    }
    for (int i = 0; i < 32; i++) {
        wl_char *c = w->chars[i];
        if (c == NULL) continue;
        char_free(c);
        g_eng.op_delete(c);
        w->chars[i] = NULL;
    }
    for (int i = 0; i < 128; i++) {
        wl_actor *a = (wl_actor *)w->actors[i];
        if (a == NULL) continue;
        actor_free(a);
        g_eng.op_delete(a);
        w->actors[i] = NULL;
    }
    for (int i = 0; i < 1024; i++) {
        wl_entity *e = w->particles[i];
        if (e == NULL) continue;
        entity_free(e);
        g_eng.op_delete(e);
        w->particles[i] = NULL;
    }
    for (int i = 0; i < 96; i++) {
        wl_entity *e = w->band96[i];
        if (e == NULL) continue;
        entity_free(e);
        g_eng.op_delete(e);
        w->band96[i] = NULL;
    }
    for (int i = 0; i < 128; i++) {
        wl_entity *e = w->band2560[i];
        if (e == NULL) continue;
        entity_free(e);
        g_eng.op_delete(e);
        w->band2560[i] = NULL;
    }
    list_free(&w->list_2788);
    list_free(&w->collide);
    list_free(&w->list_2790);
    list_free(&w->list_2794);
    list_free(&w->list_2798);
    if (w->grid != NULL) {
        g_eng.op_delete(w->grid);
        w->grid = NULL;
    }
    if (w->view != NULL) {
        view_pools_free(w->view);   /* inlined in retail (the same frees) */
        g_eng.op_delete(w->view);
        w->view = NULL;
    }
    if (w->f1050 != NULL) {
        g_eng.op_delete(w->f1050);
        w->f1050 = NULL;
    }
    if (w->event != NULL) {
        g_eng.op_delete(w->event);
        w->event = NULL;
    }
    if (w->f1058 != NULL) {
        g_eng.op_delete(w->f1058);
        w->f1058 = NULL;
    }
    uint8_t *f = w->f1040;
    if (f != NULL) {
        if (ldp(f, 0) != NULL) {
            g_eng.op_delete(ldp(f, 0));
            stp(f, 0, NULL);
        }
        g_eng.op_delete(f);
        w->f1040 = NULL;
    }
}
