/* event.c — FUN_0040b8f0 + FUN_0043e3a0.  See event.h.
 *
 * Transcribed from the asm.  0x40b8f0 inlines 0x43e3a0 with group 4 (after its own
 * "StartEvent" line); the frees go through the engine slots so rhost releases retail's
 * own surfaces / buffers / GDI objects. */
#include "event.h"

#include <stddef.h>
#include <string.h>

#include "asset_register.h"
#include "engine_calls.h"

/* sys+0xaa8: the timed-sound list 0x43e140 runs (24-byte records: buffer, volume, duration,
 * start, +0x10 state 1 start / 2 play / 3 fade out by +0x14 per tick) */
typedef struct ev_rec {
    int32_t  f00;
    int32_t  _p04[3];
    int32_t  state;   /* +0x10 */
    int32_t  f14;     /* +0x14 */
} ev_rec;
typedef struct ev_list {
    uint8_t  _p0[4];
    uint16_t n;       /* +0x4 */
    uint16_t _p6;
    ev_rec  *rec;     /* +0x8 */
} ev_list;
/* sys+0x2714: the 16 GDI object sets */
typedef struct ev_gdi {
    void   **obj;     /* +0x0 */
    uint16_t f04;     /* +0x4 */
    uint16_t n;       /* +0x6 */
    uint16_t group;   /* +0x8 */
} ev_gdi;

#if UINTPTR_MAX == 0xFFFFFFFFu
_Static_assert(sizeof(ev_rec) == 24, "event list record");
_Static_assert(offsetof(ev_list, rec) == 8, "event list records");
_Static_assert(offsetof(ev_gdi, group) == 8, "gdi set group");
#endif

static void *sysp(uint32_t off) { void *p; memcpy(&p, (const uint8_t *)g_eng.sys + off, sizeof p); return p; }

int event_run(wl_event *ev, void *f00, wl_room *room, wl_actor *actor, int32_t a5, int32_t f10, int32_t mode)
{
    (void)a5;   /* retail never reads it */
    ev->actor = actor;
    ev->f00 = f00;
    ev->mode = mode;
    ev->room = room;
    ev->f10 = f10;
    ev->unload = 0;
    int r = g_eng.h40c380(ev);
    if (r != 0) return r;
    if (ev->unload == 0) return 0;
    ev_list *l = sysp(0xaa8);
    if (l != NULL)
        for (uint16_t i = 0; i < l->n; i++) {
            ev_rec *e = &l->rec[i];
            if (e->f00 != 0 && e->state < 3) {
                e->state = 3;
                l->rec[i].f14 = 400;
            }
        }
    g_eng.dbg_out(g_eng.data(0x854798u));   /* "Unload Resource From 'StartEvent'" */
    sys_unload(g_eng.sys, 4);
    return 24;
}

void sys_unload(void *sys, uint16_t group)
{
    const uint8_t *s = sys;
    g_eng.dbg_out(g_eng.data(0x854780u));   /* "Unload Surface Sets" */
    for (uint32_t i = 0; i < 909; i++) {
        ar_sprite_slot *t; memcpy(&t, s + 0xaac + 4 * i, sizeof t);
        if (t == NULL || t->group < group) continue;
        for (uint16_t k = 0; k < t->entry_count; k++) {
            if (t->entries == NULL) continue;
            ar_sprite_entry *en = &t->entries[k];
            if (en->frames == NULL) continue;
            for (uint32_t j = 0; j < t->f_38; j++) {
                void **fr = en->frames;
                if (fr[j] != NULL) {
                    void *f = fr[j];
                    g_eng.h5b9390(f);
                    g_eng.op_delete(f);
                }
                ((void **)en->frames)[j] = NULL;
            }
            g_eng.op_delete(en->frames);
            en->frames = NULL;
        }
    }
    g_eng.dbg_out(g_eng.data(0x854770u));   /* "Unload Waves" */
    for (uint32_t i = 0; i < 465; i++) {
        wl_vpool *p; memcpy(&p, s + 0x364 + 4 * i, sizeof p);
        if (p == NULL || p->group < group || p->v == NULL) continue;
        for (int32_t j = (int32_t)p->n - 1; j >= 0; j--) {
            void *buf = p->v[j].buf;
            if (buf == NULL) continue;
            g_eng.dsb_release_5bb3a0(buf);
            g_eng.op_delete(buf);
            p->v[j].buf = NULL;
        }
        g_eng.op_delete(p->v);
        p->v = NULL;
    }
    g_eng.dbg_out(g_eng.data(0x854760u));   /* "Unload GDIs" */
    for (uint32_t i = 0; i < 16; i++) {
        ev_gdi *g; memcpy(&g, s + 0x2714 + 4 * i, sizeof g);
        if (g == NULL || g->group < group || g->obj == NULL) continue;
        for (uint16_t k = 0; k < g->n; k++)
            if (g->obj[k] != NULL) {
                g_eng.gdi_delete(g->obj[k]);
                g->obj[k] = NULL;
            }
        g_eng.op_delete(g->obj);
        g->n = 0;
        g->f04 = 0;
        g->obj = NULL;
    }
    g_eng.dbg_out(g_eng.data(0x854744u));   /* "Success Resource Unload" */
}
