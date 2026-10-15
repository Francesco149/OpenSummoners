/* scene_loop.c — FUN_0059f2c0 + its helpers.  See scene_loop.h.
 *
 * Transcribed from the asm (0x59f2c0 inlines 0x41a7f0 for its stack scene object and 0x5a3bb0
 * for save 10030; 0x5a3ff0 / 0x562ba0 inline 0x43e3a0's unload loops = sys_unload with group
 * 3 / 2; 0x560e90 / 0x4c5830 inline the fatal-log sequence = engine_fatal).  The save-file
 * reader (0x5b6060.. on a 0x414-byte stack object) stays behind slots (platform: file I/O). */
#include "scene_loop.h"

#include <stddef.h>
#include <string.h>

#include "behave_ops.h"
#include "engine_calls.h"
#include "entity_life.h"
#include "entity_move.h"
#include "event.h"
#include "room_load.h"
#include "sprite_slot.h"
#include "world_alloc.h"

#define IP(p) ((intptr_t)(p))
#define B(p) ((uint8_t *)(p))

static int32_t  ld32(const void *p, uint32_t off) { int32_t v; memcpy(&v, (const uint8_t *)p + off, 4); return v; }
static uint16_t ld16(const void *p, uint32_t off) { uint16_t v; memcpy(&v, (const uint8_t *)p + off, 2); return v; }
static void st32(void *p, uint32_t off, int32_t v) { memcpy((uint8_t *)p + off, &v, 4); }
static void st16(void *p, uint32_t off, uint16_t v) { memcpy((uint8_t *)p + off, &v, 2); }
static void *ldp(const void *p, uint32_t off) { void *v; memcpy(&v, (const uint8_t *)p + off, sizeof v); return v; }
static void stp(void *p, uint32_t off, const void *v) { memcpy((uint8_t *)p + off, &v, sizeof v); }
static const void *D(uint32_t va) { return g_eng.data(va); }
static int32_t mul(int32_t a, int32_t b) { return (int32_t)((uint32_t)a * (uint32_t)b); }

/* ---- the scene object, the tables ---- */

uint8_t *scene_obj_init(uint8_t *o)
{
    memset(o, 0, 0xa4);
    memset(o + 0xa4, 0, 0x2000);
    for (int i = 0; i < 8; i++) {
        memset(o + 0x20a4 + 0x11cu * i, 0, 0x11c);
        memset(o + 0x2984 + 0x420u * i, 0, 0x420);
    }
    st32(o, 0x4a84, 0);
    st32(o, 0x4a88, 0);
    memset(o + 0x4a8c, 0, 0x1800);
    memset(o + 0x628c, 0, 0x6000);
    st16(o, 0xc28c, 0);
    memset(o + 0xc290, 0, 0x2000);
    return o;
}

uint8_t *flag_find(uint8_t *t, int32_t id)
{
    uint32_t n = (uint32_t)ld32(t, 0x1800);
    for (uint32_t i = 0; i < n; i++)
        if (ld32(t, 12 * i) == id) return t + 12 * i;
    return NULL;
}

uint8_t *flag_add(uint8_t *t, int32_t id)
{
    uint32_t n = (uint32_t)ld32(t, 0x1800);
    uint8_t *r = NULL;
    for (uint16_t i = 0; i < n; i++)
        if (ld32(t, 12u * i) == 0) r = t + 12u * i;   /* (the last free one) */
    if (r == NULL) {
        if (n == 512) engine_fatal(0x855040u);     /* "The event flag area exceeded ..." */
        st32(t, 0x1800, (int32_t)(n + 1));
        r = t + 12 * n;
    }
    st32(r, 8, 0);
    st32(r, 4, 0);
    st32(r, 0, id);
    return r;
}

uint8_t *room_flag_set(wl_room *r, int32_t id, int32_t state)
{
    uint16_t n = r->nflags;
    uint8_t *f = NULL;
    for (uint16_t i = 0; i < n; i++)
        if (r->flags[i].id == id) { f = (uint8_t *)&r->flags[i]; break; }
    if (f == NULL) {
        if (n >= 0x400) engine_fatal(0x88df7cu);    /* "Area state over ..." */
        f = (uint8_t *)&r->flags[r->nflags];
        st32(f, 0, id);
        r->nflags++;
    }
    st32(f, 4, state);
    return f;
}

uint8_t *area_find(uint8_t *areas, int32_t id)
{
    uint16_t n = ld16(areas, 0x54004);
    for (uint16_t i = 0; i < n; i++)
        if (ld32(areas, 4 + 336u * i) == id) return areas + 4 + 336u * i;
    return NULL;
}

void area_xref(uint8_t *areas, const uint8_t *rows, uint8_t *rec)
{
    const uint8_t *mt = ldp(areas, 0);   /* the map table: 64-byte rows keyed by +0 */
    if (ld32(mt, 0) == 0) return;
    uint16_t i = 0;
    while (ld32(mt, 64u * i) != ld32(rec, 4)) {
        i++;
        if (ld32(mt, 64u * i) == 0) return;
    }
    const uint8_t *m = mt + 64u * i;
    if (ld32(rec, 0x110) == 0) st32(rec, 0x110, ld32(m, 0x2c));
    if (ld32(rec, 0x10c) == 0) st32(rec, 0x10c, ld32(m, 0x34));
    if (ld32(rec, 0x114) == 0) st32(rec, 0x114, ld32(m, 0x30));
    if (ld32(rec, 0x140) == 0) st32(rec, 0x140, ld32(m, 0x38));
    if (ld16(rec, 0x144) == 0) st16(rec, 0x144, ld16(m, 0x3c));
    if (ld16(rec, 0x146) == 0) st16(rec, 0x146, ld16(m, 0x3e));
    if (rows == NULL) return;
    /* every other area's zones that lead here become this area's reverse links */
    uint16_t n = ld16(areas, 0x54004);
    for (const uint8_t *r = rows; n != 0; n--, r += 336) {
        if (ld32(r, 0) == ld32(rec, 0)) continue;
        for (int z = 0; z < 20; z++) {
            const uint8_t *e = r + 0x1c + 12 * z;
            if (ld32(e, 4) != ld32(rec, 0)) continue;
            int32_t from = ld32(r, 0);
            uint16_t gate = ld16(e, 0), back = ld16(e, 8);
            int j;
            for (j = 0; j < 20 && ld16(rec, 0x1c + 12 * j) != back; j++) {}
            if (j < 20) continue;
            for (j = 0; j < 20 && ld16(rec, 0x1c + 12 * j) != 0; j++) {}
            if (j == 20) continue;
            st16(rec, 0x1c + 12 * j, back);
            st32(rec, 0x20 + 12 * j, from);
            st16(rec, 0x24 + 12 * j, gate);
            st16(rec, 0x26 + 12 * j, 0);
        }
    }
}

void fill_clear_mapped(uint8_t *fill)
{
    uint32_t n = (uint32_t)ld32(fill, 0x7804);
    for (uint16_t i = 0; i < n; i++)
        if (ld32(fill, 0x180c + 12u * i) != 0) st32(fill, 0x1804 + 12u * i, 0);
}

uint8_t *keep_find(uint8_t *ss, int32_t map)
{
    for (uint16_t i = 0; i < 3; i++) {
        uint8_t *k = ldp(ss, 4 + 4u * i);
        if (ld32(k, 0) == map) return k;
    }
    return NULL;
}

/* the live map NPCs of the current room into keep list k (up to the 32 characters) */
static void keep_collect(uint8_t *k)
{
    for (int i = 0; i < 32; i++) {
        const wl_char *c = (*g_eng.world)->chars[i];
        if (c->base.alive != 1 || c->base.body->active != 1) continue;
        if (ld16(c->party, 0x44a) != 0 || c->f1f8 != 1) continue;
        int32_t id = c->f1fc;
        if (id == 0 || ld32(c, 0x1dc) != 2) continue;
        uint16_t n = ld16(k, 0x84);
        st32(k, 4 + 4u * n, id);
        st16(k, 0x84, (uint16_t)(n + 1));
    }
}

void keep_snapshot(uint8_t *ss, wl_roomloop *rl)
{
    (void)rl;   /* (retail takes it and reads nothing) */
    for (int i = 0; i < 3; i++) {
        uint8_t *k = ldp(ss, 4 + 4u * i);
        if (ld32(k, 0) != 0) st16(k, 0x86, (uint16_t)(ld16(k, 0x86) + 1));
    }
    int32_t map = ld32((*g_eng.world)->roominfo, 0);
    for (uint16_t i = 0; i < 3; i++) {
        uint8_t *k = ldp(ss, 4 + 4u * i);
        if (ld32(k, 0) == map) {
            st16(k, 0x84, 0);
            st32(k, 0, ld32((*g_eng.world)->roominfo, 0));
            keep_collect(k);
            return;
        }
    }
    /* a new list: the first empty one, else the oldest */
    uint8_t *best = NULL, *k = NULL;
    for (int i = 0; i < 3; i++) {
        uint8_t *t = ldp(ss, 4 + 4u * i);
        if (ld32(t, 0) == 0) { k = t; break; }
        if (best == NULL || ld16(best, 0x86) < ld16(t, 0x86)) best = t;
    }
    if (k == NULL) k = best;
    if (k == NULL) return;
    st16(k, 0x84, 0);
    st32(k, 0, ld32((*g_eng.world)->roominfo, 0));
    st16(k, 0x86, 1);
    keep_collect(k);
}

/* ---- the party room ---- */

void pslot_reset(wl_pslot *s)
{
    s->active = 0;
    s->id = 0;
    s->code = 0;
    s->ch = NULL;
    s->order = 0;
    s->in = NULL;
    st16(s, 0x9c8, 0);
    st32(s, 0x8, 0);
}

void room_party_defaults(wl_room *r)
{
    st32(r, 0x40e0, 0);
    st32(r, 0x40e8, 0);
    st32(r, 0x40f0, 255);
    st32(r, 0x40f4, 255);
    st32(r, 0x40f8, 255);
    st32(r, 0x40fc, 255);
    st32(r, 0x4100, 0);
    st32(r, 0x40ec, 0);
}

void input_ctor(uint8_t *in)
{
    for (int i = 0; i < 64; i++) stp(in, 0xc + 4u * i, g_eng.op_new(0xc));
}

void input_dtor(uint8_t *in)
{
    for (int i = 0; i < 64; i++) {
        void *p = ldp(in, 0xc + 4u * i);
        if (p == NULL) continue;
        g_eng.op_delete(p);
        stp(in, 0xc + 4u * i, NULL);
    }
}

/* the counted array of 0x170-byte input managers (MSVC's vector delete: count before the
 * elements, destroyed last to first) */
static void inputs_free(wl_room *r)
{
    uint8_t *a = (uint8_t *)r->in;
    if (a == NULL) return;
    int32_t n = ld32(a, -4);
    for (int32_t i = n - 1; i >= 0; i--) input_dtor(a + 0x170u * (uint32_t)i);
    g_eng.op_delete(a - 4);
    r->in = NULL;
}

void room_inputs_alloc(wl_room *r, uint16_t n)
{
    inputs_free(r);
    r->nin = n;
    if (n == 0) return;
    uint8_t *p = g_eng.op_new(0x170u * n + 4);
    uint8_t *a = NULL;
    if (p != NULL) {
        st32(p, 0, n);
        a = p + 4;
        for (uint16_t i = 0; i < n; i++) input_ctor(a + 0x170u * i);
    }
    r->in = (input_mgr *)a;
}

void room_delete(wl_room *r, int32_t flags)
{
    for (int i = 0; i < 8; i++)
        if (r->slot[i] != NULL) {
            g_eng.op_delete(r->slot[i]);
            r->slot[i] = NULL;
        }
    inputs_free(r);
    if (flags & 1) g_eng.op_delete(r);
}

uint8_t *input_init(wl_room *r, uint16_t i, int32_t a, int32_t b, int32_t c)
{
    uint8_t *m = (uint8_t *)r->in + 0x170u * i;
    st32(m, 0, a);
    st32(m, 4, b);
    st32(m, 8, c);
    st16(m, 0x16c, 0);
    for (int k = 0; k < 11; k++) {
        st32(m, 0x140 + 4u * k, 0);
        st32(m, 0x114 + 4u * k, 0);
    }
    for (int k = 0; k < 64; k++) st32(ldp(m, 0xc + 4u * k), 0, 0);
    st32(m, 0x10c, 0);
    st32(m, 0x110, 0);
    return m;
}

void room_area_enter(int32_t *key, int32_t mode, wl_room *r)
{
    stp(key, 4, r);
    key[0] = mode;
    g_eng.h4c5350(key);
    g_eng.h4e59a0(key);
}

int room_has_retry(const wl_room *r) { return r->f4080 == 0 && r->f40d0 != 0; }

void room_retry_point(wl_room *r)
{
    st32(r, 0x4024, r->f40d0);
    st32(r, 0x4028, ld32(r, 0x40d4));
    st32(r, 0x402c, ld32(r, 0x40d8));
}

void room_next_set(wl_room *r, int32_t area, int32_t gate, int32_t a7)
{
    st32(r, 0x4024, area);
    st32(r, 0x4028, gate);
    st32(r, 0x402c, a7);
}

void room_party_heal(wl_room *r, int32_t all)
{
    for (int i = 0; i < 8; i++) {
        uint8_t *s = (uint8_t *)r->slot[i];
        int32_t st = ld32(s, 0x9c4);
        if (st == 1) { if (ld32(s, 0x9f0) == 100000104) continue; }
        else if (st != 2 || all == 0) continue;
        st32(s, 0xa64, ld32(s, 0xaac) + ld32(s, 0xa94) + ld32(s, 0xa68));
        st32(s, 0xa6c, ld32(s, 0xab0) + ld32(s, 0xa98) + ld32(s, 0xa70));
        memset(s + 0xe58, 0, 0x24);
    }
}

int room_gold_add(wl_room *r, int32_t n, int32_t count)
{
    if (n > 0 && ld32(r, 0x4020) <= ld32(r, 0x4014)) return 0;
    int32_t v = n + ld32(r, 0x4014);
    st32(r, 0x4014, v);
    if (n > 0) {
        if (ld32(r, 0x4020) < v) v = ld32(r, 0x4020);
        st32(r, 0x4014, v);
        if (count != 0) {
            uint32_t t = (uint32_t)ld32(r, 0x40bc) + (uint32_t)n;
            if (t > 0x7fffffffu) t = 0x7fffffffu;
            st32(r, 0x40bc, (int32_t)t);
        }
    }
    return 1;
}

void room_from_scene(wl_room *r, const uint8_t *so)
{
    st32(r, 0x40b0, ld32(so, 0x2c));
    st32(r, 0x406c, ld32(so, 0x7c));
    st32(r, 0x4104, ld32(so, 0x84));
    for (int i = 0; i < 3; i++) st16(r, 0x405c + 2u * i, ld16(so, 0x3c + 2u * i));
    st32(r, 0x4064, ld16(so, 0x88));
    st32(r, 0x4050, ld16(so, 0xa));
    st32(r, 0x4054, ld16(so, 0xc));
    st32(r, 0x4058, ld16(so, 0x80));
}

int pslot_input_set(wl_pslot *s, void *in)
{
    s->in = in;
    if (s->active != 1 || s->id == 100000104 || s->ch == NULL) return s->active - 1;
    wl_char *c = s->ch;
    if (in != NULL) {
        stp(c, 0x158a4, in);
        st32(c, 0x200, 0);
        st16(c, 0x1429c, 0);
        st16(c, 0x147a0, 0);
        st16(c, 0x1422c, 0);
        st16(c, 0x1422e, 0);
        st16(c, 0x14230, 0);
    } else {
        st32(c, 0x158a4, 0);
        st32(c, 0x200, 1);
        st16(c, 0x1429c, 0);
        st16(c, 0x147a0, 0);
        behave_set_state(c, 0);
    }
    st16(c, 0x14278, 0);
    st16(c, 0x147ac, 0);
    st16(c, 0x147f4, 0);
    st16(c, 0x14232, 0);
    st16(c, 0x14802, 0);
    st32(c, 0x14270, 0);
    st32(c, 0x1427c, 0);
    st16(c, 0x14234, 0);
    st16(c, 0x14236, 0);
    st16(c, 0x14804, 0);
    st16(c, 0x14288, 0);
    st32(c, 0x1428c, 0);
    st16(c, 0x14248, 0);
    st16(c, 0x14290, 0);
    st32(c, 0x14294, 0);
    st32(c, 0x1423c, 0);
    st32(c, 0x14810, 0);
    st32(c, 0x14814, 0);
    st16(c, 0x14830, 0);
    st16(c, 0x1482e, 0);
    st32(c, 0x14274, 0);
    st16(c, 0x1428a, 0);
    for (uint16_t k = 0; k < ld16(c, 0x14228); k++)
        st16(c, 0xcb40 + 320u * k, ld16(c, 0xcb44 + 320u * k));
    wl_body *b = c->base.body;
    int32_t y = b->y;
    c->platform = NULL;
    return entity_move_vertical(&c->base, b, 1, 0, 0, &y, (wl_entity **)&c->platform);
}

/* ---- the room loop object ---- */

/* a 0x24-byte pool object whose +0xc is {buffer (0x5a0), 40, u16 0, u16 0} */
static void *pool_obj_new(void)
{
    uint8_t *o = g_eng.op_new(0x24);
    if (o == NULL) return NULL;
    uint8_t *q = g_eng.op_new(0xc);
    if (q != NULL) {
        st32(q, 4, 40);
        stp(q, 0, g_eng.op_new(0x5a0));
        st16(q, 8, 0);
        st16(q, 0xa, 0);
        stp(o, 0xc, q);
    } else {
        stp(o, 0xc, NULL);
    }
    return o;
}

wl_roomloop *roomloop_new(wl_roomloop *rl)
{
    *g_eng.world = NULL;
    st32(rl, 0xe8, 0);
    stp(rl, 0x11c, pool_obj_new());
    stp(rl, 0x120, pool_obj_new());
    void *p = pool_obj_new();
    st32(rl, 0xe0, 0);
    stp(rl, 0x124, p);
    memset(B(rl) + 0x14, 0, 0x2c);
    return rl;
}

void pool_obj_delete(uint8_t *p, int32_t flags)
{
    void **q = ldp(p, 0xc);
    if (q != NULL) {
        g_eng.op_delete(*q);
        g_eng.op_delete(q);
        stp(p, 0xc, NULL);
    }
    if (flags & 1) g_eng.op_delete(p);
}

void world_delete(wl_world *w, int32_t flags)
{
    world_free(w);
    if (flags & 1) g_eng.op_delete(w);
}

void roomloop_pools_free(wl_roomloop *rl)
{
    wl_world *w = *g_eng.world;
    if (w != NULL) {
        world_free(w);
        g_eng.op_delete(w);
        *g_eng.world = NULL;
    }
    uint8_t *h = ldp(rl, 0xe8);
    if (h == NULL) return;
    for (uint16_t i = 0; i < ld16(h, 4); i++) {
        uint8_t *r = ldp((uint8_t *)ldp(h, 0) + 4u * i, 0);
        if (r == NULL) continue;
        if (ldp(r, 0x48) != NULL) {
            for (uint16_t j = 0; j < ld16(r, 0x4c); j++) {
                void *o = ldp((uint8_t *)ldp(r, 0x48) + 4u * j, 0);
                if (o == NULL) continue;
                ctrl_geometry_free(o);
                g_eng.op_delete(o);
                stp((uint8_t *)ldp(r, 0x48) + 4u * j, 0, NULL);
            }
            if (ldp(r, 0x48) != NULL) {
                g_eng.op_delete(ldp(r, 0x48));
                stp(r, 0x48, NULL);
            }
        }
        g_eng.op_delete(r);
        stp((uint8_t *)ldp(h, 0) + 4u * i, 0, NULL);
    }
    if (ldp(h, 0) != NULL) {
        g_eng.op_delete(ldp(h, 0));
        stp(h, 0, NULL);
    }
    g_eng.op_delete(h);
    stp(rl, 0xe8, NULL);
}

void roomloop_free(wl_roomloop *rl)
{
    wl_world *w = *g_eng.world;
    if (w != NULL) {
        world_free(w);
        g_eng.op_delete(w);
        *g_eng.world = NULL;
    }
    void *h = ldp(rl, 0xe8);
    if (h != NULL) {
        g_eng.h56c640(h);
        g_eng.op_delete(h);
        stp(rl, 0xe8, NULL);
    }
    for (uint32_t o = 0x11c; o <= 0x124; o += 4) {
        uint8_t *p = ldp(rl, o);
        if (p == NULL) continue;
        pool_obj_delete(p, 0);
        g_eng.op_delete(p);
        stp(rl, o, NULL);
    }
    if (ldp(rl, 0xe0) != NULL) {
        g_eng.op_delete(ldp(rl, 0xe0));
        stp(rl, 0xe0, NULL);
    }
}

/* ---- the cleanups ---- */

void frame_cache_clear(uint8_t *c)
{
    for (uint16_t i = 0; i < ld16(c, 4); i++) {
        uint8_t *r = (uint8_t *)ldp(c, 0) + 28u * i;
        void *f = ldp(r, 24);
        if (f != NULL) {
            g_eng.h5b9390(f);
            g_eng.op_delete(f);
            stp((uint8_t *)ldp(c, 0) + 28u * i, 24, NULL);
        }
        r = (uint8_t *)ldp(c, 0) + 28u * i;
        for (uint32_t o = 0; o < 24; o += 4) st32(r, o, 0);
    }
}

void room_end_cleanup(uint8_t *ss)
{
    wl_roomloop *rl = ldp(ss, 0);
    if (rl->rooms != NULL)
        for (uint16_t i = 0; i < rl->nrooms; i++)
            for (int k = 0; k < 8; k++) rl->rooms[i].slot[k]->ch = NULL;
    for (int i = 0; i < 64; i++) entity_release((*g_eng.world)->band64[i], 0);
    for (int i = 0; i < 32; i++) char_release((*g_eng.world)->chars[i]);
    for (int i = 0; i < 128; i++) actor_release((wl_actor *)(*g_eng.world)->actors[i]);
    for (int i = 0; i < 1024; i++) entity_release((*g_eng.world)->particles[i], 1);
    for (int i = 0; i < 96; i++) entity_release((*g_eng.world)->band96[i], 0);
    for (int i = 0; i < 128; i++) entity_release((*g_eng.world)->band2560[i], 0);
    roomloop_pools_free(rl);
    wl_roomloop *r0 = ldp(ss, 0);
    if (r0 != NULL) {
        roomloop_free(r0);
        g_eng.op_delete(r0);
        stp(ss, 0, NULL);
    }
    for (int i = 0; i < 25; i++) st32(ldp(g_eng.sys, 0x26b0 + 4u * i), 0x10, 0);
    g_eng.dbg_out(D(0x8a2c8cu));   /* "Unload Resource From 'EndArea'" */
    sys_unload(g_eng.sys, 3);
    g_eng.dbg_out(D(0x8a2c7cu));   /* "Clear All Cash" */
    frame_cache_clear(ldp(g_eng.sys, 0x2754));
}

void session_end(uint8_t *ss)
{
    wl_roomloop *rl = ldp(ss, 0);
    if (rl != NULL) {
        if (*g_eng.world != NULL) {
            world_delete(*g_eng.world, 1);
            *g_eng.world = NULL;
        }
        if (ldp(rl, 0xe8) != NULL) {
            pool_head_delete(ldp(rl, 0xe8), 1);
            stp(rl, 0xe8, NULL);
        }
        for (uint32_t o = 0x11c; o <= 0x124; o += 4)
            if (ldp(rl, o) != NULL) {
                pool_obj_delete(ldp(rl, o), 1);
                stp(rl, o, NULL);
            }
        if (ldp(rl, 0xe0) != NULL) {
            g_eng.op_delete(ldp(rl, 0xe0));
            stp(rl, 0xe0, NULL);
        }
        g_eng.op_delete(rl);
        stp(ss, 0, NULL);
    }
    for (int i = 0; i < 3; i++)
        if (ldp(ss, 4 + 4u * i) != NULL) {
            g_eng.op_delete(ldp(ss, 4 + 4u * i));
            stp(ss, 4 + 4u * i, NULL);
        }
    g_eng.dbg_out(D(0x8a0bfcu));   /* "Unload Resource From '_TerminateGame'" */
    sys_unload(g_eng.sys, 2);
    if (ldp(ss, 0x10) != NULL) { g_eng.op_delete(ldp(ss, 0x10)); stp(ss, 0x10, NULL); }
    if (ldp(ss, 0x14) != NULL) { g_eng.op_delete(ldp(ss, 0x14)); stp(ss, 0x14, NULL); }
}

void obj_free(uint8_t *o)
{
    uint8_t *h = ldp(o, 0);
    if (h != NULL) {
        for (uint16_t i = 0; i < ld16(h, 4); i++) {
            uint8_t *e = ldp((uint8_t *)ldp(h, 0) + 4u * i, 0);
            if (e == NULL) continue;
            if (ldp(e, 0x48) != NULL) {
                for (uint16_t j = 0; j < ld16(e, 0x4c); j++) {
                    void *x = ldp((uint8_t *)ldp(e, 0x48) + 4u * j, 0);
                    if (x == NULL) continue;
                    g_eng.h5a3670(x);
                    g_eng.op_delete(x);
                    stp((uint8_t *)ldp(e, 0x48) + 4u * j, 0, NULL);
                }
                if (ldp(e, 0x48) != NULL) {
                    g_eng.op_delete(ldp(e, 0x48));
                    stp(e, 0x48, NULL);
                }
            }
            g_eng.op_delete(e);
            stp((uint8_t *)ldp(h, 0) + 4u * i, 0, NULL);
        }
        if (ldp(h, 0) != NULL) {
            g_eng.op_delete(ldp(h, 0));
            stp(h, 0, NULL);
        }
        g_eng.op_delete(h);
        stp(o, 0, NULL);
    }
    uint8_t *q = ldp(o, 4);
    if (q != NULL) {
        for (int i = 0; i < 64; i++)
            if (ldp(q, 0xc + 4u * i) != NULL) {
                g_eng.op_delete(ldp(q, 0xc + 4u * i));
                stp(q, 0xc + 4u * i, NULL);
            }
        g_eng.op_delete(q);
        stp(o, 4, NULL);
    }
}

void obj_delete(uint8_t *o, int32_t flags)
{
    obj_free(o);
    if (flags & 1) g_eng.op_delete(o);
}

/* ---- the room snapshot ---- */

void room_to_scene(const wl_room *r, uint8_t *so)
{
    st16(so, 0, 8);
    st16(so, 8, ld16(r, 0x4010));
    st32(so, 4, ld32(r, 0x4014));
    st32(so, 0x1c, ld32(r, 0x401c));
    st16(so, 0xa, (uint16_t)ld32(r, 0x4050));
    st16(so, 0xc, (uint16_t)ld32(r, 0x4054));
    st16(so, 0x80, (uint16_t)ld32(r, 0x4058));
    st32(so, 0x10, ld32(r, 0x4024));
    st32(so, 0x14, ld32(r, 0x4028));
    st32(so, 0x18, ld32(r, 0x402c));
    st32(so, 0x20, ld32(r, 0x40d0));
    st32(so, 0x24, ld32(r, 0x40d4));
    st32(so, 0x28, ld32(r, 0x40d8));
    st32(so, 0x2c, ld32(r, 0x40b0));
    st32(so, 0x30, ld32(r, 0x40b4));
    st32(so, 0x34, ld32(r, 0x40b8));
    st32(so, 0x38, ld32(r, 0x40bc));
    st32(so, 0x8c, ld32(r, 0x40c0));
    st32(so, 0x90, ld32(r, 0x40c4));
    st32(so, 0x94, ld32(r, 0x40c8));
    st32(so, 0x98, ld32(r, 0x40cc));
    st32(so, 0x9c, ld32(r, 0x40dc));
    uint32_t now = g_eng.tick();
    st32(so, 0x7c, (int32_t)((now - (uint32_t)ld32(r, 0x4068)) / 1000u + (uint32_t)ld32(r, 0x406c)));
    st32(so, 0x84, ld32(r, 0x4104));
    for (int i = 0; i < 3; i++) st16(so, 0x3c + 2u * i, ld16(r, 0x405c + 2u * i));
    st16(so, 0x88, (uint16_t)ld32(r, 0x4064));
    for (uint16_t i = 0; i < ld16(r, 0x4010); i++) {
        st32(so, 0xa4 + 16u * i, ld32(r, 0x2010 + 16u * i));
        st32(so, 0xa8 + 16u * i, ld32(r, 0x2014 + 16u * i));
        st16(so, 0xb0 + 16u * i, ld16(r, 0x2018 + 16u * i));
        so[0xb2 + 16u * i] = ((const uint8_t *)r)[0x201a + 16u * i];
    }
    const uint8_t *eq = D(0x628708u);   /* 18 rows of 140 B: +0 the mark */
    for (int k = 0; k < 8; k++) {
        const uint8_t *s = (const uint8_t *)r->slot[k];
        uint8_t *m = so + 0x20a4 + 0x11cu * k, *x = so + 0x2984 + 0x420u * k;
        int lead = (const void *)s == (const void *)r->leader;
        memset(m, 0, 0x11c);
        memset(x, 0, 0x420);
        st16(m, 0, ld16(s, 0xa0c));
        st16(m, 8, ld16(s, 0x9c4));
        if (ld32(s, 0x9c4) == 0) continue;
        st32(m, 4, ld32(s, 0x9f0));
        const uint8_t *t = D(0x6b6ea8u);   /* the dramatist row's code, else the slot's */
        int32_t code = ld32(s, 4);
        for (uint16_t i = 0; ld32(t, 52u * i) != 0; i++)
            if (ld32(t, 52u * i) == ld32(s, 0x9f0)) { code = ld32(t, 52u * i + 4); break; }
        st32(m, 0x54, code);
        st16(m, 0xa, (uint16_t)lead);
        st32(m, 0xc, ld32(s, 0x9f8));
        st32(m, 0x1c, ld32(s, 0xa64));
        st32(m, 0x20, ld32(s, 0xa68));
        st32(m, 0x60, ld32(s, 0xaac));
        st32(m, 0x24, ld32(s, 0xa6c));
        st32(m, 0x28, ld32(s, 0xa70));
        st32(m, 0x64, ld32(s, 0xab0));
        st32(m, 0x2c, ld32(s, 0xa74));
        st32(m, 0x30, ld32(s, 0xa78));
        st32(m, 0x34, ld32(s, 0xa7c));
        st32(m, 0x38, ld32(s, 0xa80));
        st32(m, 0x68, ld32(s, 0xa9c));
        st32(m, 0x6c, ld32(s, 0xaa0));
        st32(m, 0x70, ld32(s, 0xaa4));
        st32(m, 0x74, ld32(s, 0xaa8));
        st32(m, 0x3c, ld32(s, 0xaf0));
        st32(m, 0x40, ld32(s, 0xae8));
        st32(m, 0x58, ld32(s, 0xaec));
        st32(m, 0x44, ld32(s, 0xaf8));
        st32(m, 0x48, ld32(s, 0xafc));
        for (int i = 0; i < 3; i++) st16(m, 0x78 + 2u * i, ld16(s, 0xb48 + 2u * i));
        st32(m, 0x5c, ld32(s, 0xb50));
        st16(m, 0x50, ld16(s, 0xb54));
        for (uint16_t i = 0; i < ld16(s, 0x9c8); i++) st32(m, 0x9c + 4u * i, ld32(s, 0x9cc + 4u * i));
        st16(m, 0x98, ld16(s, 0x9c8));
        for (uint16_t i = 0; i < ld16(s, 0xb54); i++) {
            st32(x, 12u * i, ld32(s, 0xb58 + 12u * i));
            st32(x, 4 + 12u * i, ld32(s, 0xb5c + 12u * i));
            st32(x, 8 + 12u * i, ld16(s, 0xb60 + 12u * i));
        }
        st16(m, 0x18, 0);
        for (int i = 0; i < 18; i++) {
            uint16_t w = ld16(s, 0xe58 + 2u * i);
            int32_t mark = ld32(eq, 140u * i);
            if (w == 0 || mark == 0) continue;
            uint16_t n = ld16(m, 0x18);
            st32(x, 0x300 + 16u * n, mark);
            st16(x, 0x304 + 16u * n, w);
            st16(x, 0x30c + 16u * n, ld16(s, 0xe7c + 2u * i));
            st32(x, 0x308 + 16u * n, ld32(s, 0xea0 + 4u * i));
            st16(m, 0x18, (uint16_t)(n + 1));
        }
        st32(m, 0x10, ld32(s, 0x9fc));
        st32(m, 0x14, ld32(s, 0xa00));
    }
    const uint8_t *fill = ldp(r, 0x40ac);
    if (fill != NULL) {
        st32(so, 0x4a84, ld32(fill, 0x1800));
        st32(so, 0x4a88, ld32(fill, 0x7804));
        for (uint16_t i = 0; (uint32_t)i < (uint32_t)ld32(fill, 0x1800); i++) {
            st32(so, 0x4a8c + 12u * i, ld32(fill, 12u * i));
            st32(so, 0x4a90 + 12u * i, ld32(fill, 4 + 12u * i));
            st16(so, 0x4a94 + 12u * i, ld16(fill, 8 + 12u * i));
        }
        for (uint16_t i = 0; (uint32_t)i < (uint32_t)ld32(fill, 0x7804); i++) {
            st32(so, 0x628c + 12u * i, ld32(fill, 0x1804 + 12u * i));
            st32(so, 0x6290 + 12u * i, ld32(fill, 0x1808 + 12u * i));
            st32(so, 0x6294 + 12u * i, ld32(fill, 0x180c + 12u * i));
        }
    }
    st16(so, 0xc28c, r->nflags);
    for (uint16_t i = 0; i < r->nflags; i++) {
        st32(so, 0xc290 + 8u * i, r->flags[i].id);
        st32(so, 0xc294 + 8u * i, r->flags[i].state);
    }
}

/* ---- the save reader ---- */

int file_read_section(void *rd, void *dst, int32_t n, uint32_t cap)
{
    memset(dst, 0, cap);
    if (n == 0) return 0;
    return g_eng.h5b6340(rd, IP(dst), n);
}

/* the scene object's sections from the save (the header's sizes; zero-filled to capacity) */
static void read_save(void *rd, const int32_t *hdr, uint8_t *so)
{
    file_read_section(rd, so, hdr[0], 0xa4);
    for (uint16_t n = 0; n < ld16(so, 8); n++) file_read_section(rd, so + 0xa4 + 16u * n, hdr[1], 0x10);
    for (uint16_t i = 0; i < ld16(so, 0); i++) {
        uint8_t *m = so + 0x20a4 + 0x11cu * i;
        file_read_section(rd, m, hdr[2], 0x11c);
        for (uint16_t j = 0; j < ld16(m, 0x50); j++)
            file_read_section(rd, so + 0x2984 + 0x420u * i + 12u * j, hdr[3], 0xc);
        for (uint16_t j = 0; j < ld16(m, 0x18); j++)
            file_read_section(rd, so + 0x2c84 + 0x420u * i + 16u * j, hdr[137], 0x10);
    }
    file_read_section(rd, so + 0x4a84, hdr[4], 8);
    for (uint16_t i = 0; (uint32_t)i < (uint32_t)ld32(so, 0x4a84); i++)
        file_read_section(rd, so + 0x4a8c + 12u * i, hdr[5], 0xc);
    for (uint16_t i = 0; (uint32_t)i < (uint32_t)ld32(so, 0x4a88); i++)
        file_read_section(rd, so + 0x628c + 12u * i, hdr[136], 0xc);
    file_read_section(rd, so + 0xc28c, hdr[6], 2);
    for (uint16_t i = 0; i < ld16(so, 0xc28c); i++) file_read_section(rd, so + 0xc290 + 8u * i, hdr[7], 8);
}

/* the slots reset and the room record's defaults (both start paths) */
static void room_reset(wl_room *r, uint8_t *fill)
{
    for (uint16_t i = 0; i < 8; i++) {
        st16(r->slot[i], 0xa0c, i);
        pslot_reset(r->slot[i]);
        st32(r, 0x4084 + 4u * i, 1);
    }
    st32(r, 0x40a4, 1);
    st16(r, 0x40a8, 0);
    st32(r, 0x4018, 1);
    st32(r, 0x200c, 0);
    st16(r, 0x4010, 0);
    st32(r, 0x4014, 0);
    st32(r, 0x401c, 0);
    st32(r, 0x4050, 0);
    st32(r, 0x4054, 3);
    st32(r, 0x4058, 0);
    stp(r, 0x40ac, fill);
    st16(r, 0x2008, 0);
    for (uint32_t o = 0x40b0; o <= 0x40cc; o += 4) st32(r, o, 0);
    st32(r, 0x40dc, 0);
    st32(r, 0x40e4, 0);
    room_party_defaults(r);
    st32(r, 0x4104, 0);
    room_inputs_alloc(r, 0);
    st32(r, 0x405c, 0);
    st16(r, 0x4060, 0);
    st16(r, 0x405c, 0);
    st16(r, 0x4064, 1);
    st16(r, 0x405e, 0);
    st16(r, 0x4060, 0);
    st16(r, 0x4062, 1);
    st32(r, 0x4064, 0);
    for (int i = 0; i < 3; i++) {
        st32(r, 0x410c + 8u * i, 0);
        st32(r, 0x4108 + 8u * i, 1);
    }
    st32(r, 0x4068, (int32_t)g_eng.tick());
    st32(r, 0x406c, 0);
}

/* load save `save`: 0 when it is missing / not this save */
static int load_save(uint8_t *ss, wl_room *r, int32_t save, uint16_t sub, uint8_t *so)
{
    uint8_t rd[0x414];
    char file[512];
    int32_t hdr[151], len = 0, slot = 0;
    g_eng.h5b6060(rd);
    g_eng.h41cfd0(save, sub, file, &slot);
    int ok;
    if (slot == 9999) {
        ok = g_eng.h5b61b0(rd, IP(file)) != 0;
    } else {
        void *const *banks = (void *const *)D(0x8a93f0u);
        ok = 1;
        if (save == 10040 && g_eng.h5b6bc0(banks[slot], IP(file)) == 0) ok = 0;
        if (ok && g_eng.h5b6200(rd, IP(banks[slot])) == 0) ok = 0;
    }
    if (ok) {
        g_eng.h5b6340(rd, IP(&len), 4);
        file_read_section(rd, hdr, len, 0x25c);
        ok = hdr[139] == 10001 && hdr[140] == save;
    }
    if (!ok) {
        g_eng.h5b60d0(rd);
        return 0;
    }
    read_save(rd, hdr, so);
    g_eng.h5b64b0(rd);
    g_eng.h5b60d0(rd);
    room_reset(r, ldp(ss, 0x14));
    g_eng.h561d20(r, IP(so));
    return 1;
}

int32_t scene_loop(uint8_t *ss, int32_t save, uint16_t sub, int32_t demo, int32_t *out, int32_t start)
{
    uint8_t so[0xe290];   /* the scene object (retail: on the stack, 0x41a7f0 inlined) */
    scene_obj_init(so);
    int32_t wide = 0, f2c = 0, key[2];
    wl_room *r = g_eng.op_new(0x4120);
    if (r != NULL) {
        for (int i = 0; i < 8; i++) r->slot[i] = g_eng.op_new(0xeec);
        r->in = NULL;
    }
    *out = 0;
    stp(ss, 0x10, g_eng.op_new(0x5400c));
    stp(ss, 0x14, g_eng.op_new(0x7808));
    for (int k = 0; k < 3; k++) {
        uint8_t *p = g_eng.op_new(0x88);
        stp(ss, 4 + 4u * k, p);
        st32(p, 0, 0);
        st16(p, 0x84, 0);
    }
    *(uint16_t *)(uintptr_t)D(0x8a9400u) = 0;
    *(int32_t *)(uintptr_t)D(0x8a9408u) = 0;
    /* the area table */
    uint8_t *ar = ldp(ss, 0x10);
    stp(ar, 0, D(0x693848u));
    st32(ar, 0x54008, 0);
    st16(ar, 0x54004, 0);
    const uint8_t *rows = D(0x6940c8u);
    if (ld32(rows, 0) != 0)
        do st16(ar, 0x54004, (uint16_t)(ld16(ar, 0x54004) + 1));
        while (ld32(rows, 336u * ld16(ar, 0x54004)) != 0);
    for (uint16_t m = 0; m < ld16(ar, 0x54004); m++) {
        memcpy(ar + 4 + 336u * m, rows + 336u * m, 0x150);
        area_xref(ar, rows, ar + 4 + 336u * m);
    }
    uint8_t *fill = ldp(ss, 0x14);
    st32(fill, 0x1800, 0);
    st32(fill, 0x7804, 0);
    uint8_t *f = flag_find(ldp(ss, 0x14), 100100101);
    if (f == NULL) f = flag_add(ldp(ss, 0x14), 100100101);
    st32(f, 0, 0);
    f = flag_find(ldp(ss, 0x14), 100100102);
    if (f == NULL) f = flag_add(ldp(ss, 0x14), 100100102);
    st32(f, 4, 20);
    st32(f, 8, 0);
    if (save == 0) {
        room_reset(r, ldp(ss, 0x14));
        st32(r, 0x4104, start);
        key[0] = 0;
        stp(key, 4, r);
        g_eng.h4c5350(key);
        g_eng.h4e59a0(key);
    } else {
        if (!load_save(ss, r, save, sub, so)) goto end;
        wide = 1;
    }
    room_to_scene(r, so);
    g_eng.h41a890(so, 10020, 0, 0, 0);
    if (save == 10020) {
        if (demo != 0) {
            uint8_t *sc = g_eng.op_new(0xe290);
            if (sc != NULL) sc = scene_obj_init(sc);
            uint8_t *blk = g_eng.op_new(0x7808);
            if (g_eng.h41b670(sc, 10030, 0, 0) != 0) {
                st32(blk, 0x1800, 0);
                st32(blk, 0x7804, 0);
                for (uint16_t i = 0; (uint32_t)i < (uint32_t)ld32(sc, 0x4a84); i++)
                    g_eng.h4c5af0(blk, ld32(sc, 0x4a8c + 12u * i), ld32(sc, 0x4a90 + 12u * i), ld16(sc, 0x4a94 + 12u * i));
                for (uint16_t i = 0; (uint32_t)i < (uint32_t)ld32(sc, 0x4a88); i++)
                    g_eng.h52fd80(blk, ld32(sc, 0x628c + 12u * i), ld32(sc, 0x6290 + 12u * i), ld32(sc, 0x6294 + 12u * i));
                g_eng.h5a3870(blk, IP(ldp(ss, 0x14)));
                room_from_scene(r, sc);
            }
            if (sc != NULL) g_eng.op_delete(sc);
            if (blk != NULL) g_eng.op_delete(blk);
        }
    } else if (save == 10030) {
        fill_clear_mapped(ldp(ss, 0x14));
    }
    if (start != 0) st32(r, 0x4104, start);
    else if (ld32(r, 0x4104) == 0) st32(r, 0x4104, 1010);
    room_inputs_alloc(r, 1);
    input_init(r, 0, 1, 1, 0);
    room_area_enter(key, 1, r);
    for (;;) {
        int32_t area = ld32(r, 0x4024), gate = ld32(r, 0x4028), a7 = ld32(r, 0x402c);
        uint8_t *rinfo = area_find(ldp(ss, 0x10), area);
        if (rinfo == NULL) break;
        room_flag_set(r, area, 1);
        uint8_t *keep = keep_find(ss, area);
        if (keep != NULL && ld16(keep, 0x86) > 2) {
            st32(keep, 0, 0);
            keep = NULL;
        }
        if (r->leader != NULL) pslot_input_set(r->leader, r->in);
        wl_roomloop *rl = g_eng.op_new(0x910);
        if (rl != NULL) rl = roomloop_new(rl);
        stp(ss, 0, rl);
        int32_t cfg5c = ld32((const void *)(intptr_t)ld32((const void *)(intptr_t)ld32(g_eng.sys, 0x320), 0), 0x5c);
        int32_t res = load_room(rl, r, 1, ldp(ss, 0x10), rinfo, gate, a7, keep, ldp(ss, 0x14), 20, wide, f2c, 0, NULL,
                                cfg5c, ld32(r, 0x4104));
        wide = 0;
        f2c = 0;
        rl = ldp(ss, 0);
        switch (res) {
        case 1: case 2: case 3: case 10:
            room_next_set(r, ld32(rl, 0x900), ld32(rl, 0x904), ld32(rl, 0x908));
            break;
        case 4: case 5: {
            if (room_has_retry(r)) {
                room_retry_point(r);
                room_party_heal(r, 1);
                room_gold_add(r, mul(-100, ld32(r, 0x4014)) / 1000, 0);
                *out = 1;
            }
            uint32_t d = (uint32_t)ld32(r, 0x40b0) + 1;
            if (d > 0x7fffffffu) d = 0x7fffffffu;
            st32(r, 0x40b0, (int32_t)d);
            uint8_t *sc = g_eng.op_new(0xe290);
            if (sc != NULL) sc = scene_obj_init(sc);
            g_eng.h59cfc0(sc, r, 0, 0);
            g_eng.h41a890(sc, 10030, 0, 0, 0);
            if (sc != NULL) g_eng.op_delete(sc);
            break; }
        default: break;
        }
        if (res == 10) f2c = ld32(ldp(ss, 0), 0x90c);
        keep_snapshot(ss, ldp(ss, 0));
        room_end_cleanup(ss);
        if (res == 1 || res == 2 || res == 10) {
            const uint8_t *nx = area_find(ldp(ss, 0x10), ld32(r, 0x4024));
            if (nx != NULL && ld32(rinfo, 4) != ld32(nx, 4)) fill_clear_mapped(ldp(ss, 0x14));
            continue;
        }
        if (res == 3) {
            uint8_t *o = g_eng.op_new(8);
            if (o != NULL) { st32(o, 0, 0); st32(o, 4, 0); }
            int32_t v = g_eng.h5a00c0(o);
            if (o != NULL) obj_delete(o, 1);
            if (v != 6) continue;
            f2c = 6;
            break;
        }
        f2c = res;
        break;
    }
end:
    session_end(ss);
    if (r != NULL) room_delete(r, 1);
    return f2c;
}
