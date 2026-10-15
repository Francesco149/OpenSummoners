/* room_load.c — the room setup 0x586010 + its helpers.  See room_load.h.
 *
 * Transcribed from the asm.  Retail re-reads the world pointer 0x8a9b50 at every use; so does
 * the port (W).  The heap calls go through the engine's allocator in retail's order. */
#include "room_load.h"

#include <stddef.h>
#include <string.h>

#include "camera_follow.h"
#include "char_physics.h"
#include "engine_calls.h"
#include "entity_life.h"
#include "event.h"
#include "gimmick.h"
#include "grid_refresh.h"
#include "npc_ai.h"
#include "rng.h"
#include "room_loop.h"
#include "world_alloc.h"

#define IP(p) ((intptr_t)(p))
#define W (*g_eng.world)

static int32_t ld32(const void *p, uint32_t off) { int32_t v; memcpy(&v, (const uint8_t *)p + off, 4); return v; }
static uint16_t ld16(const void *p, uint32_t off) { uint16_t v; memcpy(&v, (const uint8_t *)p + off, 2); return v; }
static void st32(void *p, uint32_t off, int32_t v) { memcpy((uint8_t *)p + off, &v, 4); }
static void st16(void *p, uint32_t off, uint16_t v) { memcpy((uint8_t *)p + off, &v, 2); }
static void *ldp(const void *p, uint32_t off) { void *v; memcpy(&v, (const uint8_t *)p + off, sizeof v); return v; }
static void stp(void *p, uint32_t off, const void *v) { memcpy((uint8_t *)p + off, &v, sizeof v); }
static const char *S(uint32_t va) { return (const char *)g_eng.data(va); }
#define TRAILER 0x8a9b6cu

/* the loading log: 0x406440 / 0x408b90 on 0x8a6620 while 0x8a6b54 is 0 */
static void step_log(uint32_t msg)
{
    if (*g_eng.quiet == 0) g_eng.log_fail_406440(g_eng.dlog, S(msg), S(TRAILER));
}
static void step_log2(uint32_t msg)
{
    if (*g_eng.quiet == 0) g_eng.h408b90(g_eng.dlog, IP(S(msg)), 0, IP(S(TRAILER)));
}

void entity_bodies_alloc(wl_entity *e, uint16_t n)
{
    if (e->body != NULL) {
        g_eng.op_delete(e->body);
        e->body = NULL;
    }
    void *b = g_eng.op_new((uint32_t)n * 0x294u);
    e->nbody = n;
    e->body = b;
    e->alive = 0;
}

void char_links_clear(wl_char *c)
{
    memset(c->linked, 0, sizeof c->linked);
    c->fc824[0] = 0;
    c->fc824[1] = 0;
    c->carrier = NULL;
    st32(c, 0xc79c, 0);
    st32(c, 0xc7a0, 0);
    c->held = NULL;
}

void view_pools_free(void *view)
{
    if (ldp(view, 0x54) == NULL) return;
    for (uint16_t i = 0; i < ld16(view, 0x58); i++) {
        void *p = ldp((uint8_t *)ldp(view, 0x54) + 8u * i, 4);
        if (p != NULL) {
            g_eng.op_delete(p);
            stp((uint8_t *)ldp(view, 0x54) + 8u * i, 4, NULL);
        }
    }
    void *t = ldp(view, 0x54);
    if (t != NULL) {
        g_eng.op_delete(t);
        stp(view, 0x54, NULL);
    }
}

void shake_clear(void *s)
{
    st32(s, 0x0, 0);
    st32(s, 0x4, 0);
    st32(s, 0x8, 0);
    st16(s, 0xc, 0);
    st16(s, 0xe, 0);
    st32(s, 0x10, 0);
}

void volume_fade(uint8_t *f, int32_t hold)
{
    int32_t q = (int32_t)((uint32_t)ld32(f, 0x24) * 20u) / 1000;
    int32_t v;
    switch (ld32(f, 0x20)) {
    case 1:
        st32(f, 0x28, 0);
        break;
    case 2:
        if (hold != 0) break;
        v = (int32_t)((uint32_t)ld32(f, 0x28) + (uint32_t)q);
        if (v > 1000) v = 1000;
        st32(f, 0x28, v);
        if (v == 1000) st32(f, 0x20, 0);
        break;
    case 3:
        if (hold != 0) break;
        v = (int32_t)((uint32_t)ld32(f, 0x28) - (uint32_t)q);
        if (v < 0) v = 0;
        st32(f, 0x28, v);
        if (v == 0) st32(f, 0x20, 0);
        break;
    default:
        break;
    }
}

int32_t char_map_id(const wl_char *c)
{
    if (c->base.alive != 1 || c->base.body->active != 1 || c->party->busy != 0) return 0;
    return c->f1f8 == 1 ? c->f1fc : 0;
}

int char_map_npc(const wl_char *c)
{
    return c->f1f8 == 1 && c->base.faction == 2;
}

void pslot_status_clear(wl_pslot *s)
{
    const uint8_t *t = g_eng.data(0x628768u);   /* 18 rows of 0x8c bytes */
    for (int k = 0; k < 18; k++)
        if (ld32(t, 0x8cu * (uint32_t)k) != 0 && s->fe58[k] != 0) s->fe58[k] = 0;
}

wl_actor *gate_actor(int32_t id)
{
    for (uint32_t i = 0; i < 128; i++) {
        const wl_actor *a = (const wl_actor *)g_world->actors[i];
        if (a->base.alive != 0 && a->zone_kind != 0 && (a->zone == id || id == 0))
            return (wl_actor *)g_world->actors[i];
    }
    return NULL;
}

int sprite_slot_ready(const void *slot)
{
    return ld32(ldp(slot, 0), 0) != 0;
}

void sprite_slot_tint(void *slot, int32_t r, int32_t g, int32_t b, const uint8_t *lut)
{
    st32(slot, 0xc, r);
    st32(slot, 0x10, g);
    st32(slot, 0x8, 1);
    st32(slot, 0x14, b);
    stp(slot, 0x18, lut);
}

void timed_sounds_fade(void *list, int32_t rate)
{
    for (uint16_t i = 0; i < ld16(list, 4); i++) {
        uint8_t *e = (uint8_t *)ldp(list, 8) + 24u * i;
        if (ld32(e, 0) != 0 && ld32(e, 0x10) < 3) {
            st32(e, 0x10, 3);
            st32((uint8_t *)ldp(list, 8) + 24u * i, 0x14, rate);
        }
    }
}

int fill_lookup(const uint8_t *tbl, int32_t id, int32_t *out)
{
    uint32_t n = (uint32_t)ld32(tbl, 0x7804);
    for (uint32_t i = 0; i < n; i++) {
        const uint8_t *r = tbl + 0x1804 + 12u * i;
        if (ld32(r, 0) == id) {
            *out = ld32(r, 4);
            return 1;
        }
    }
    return 0;
}

wl_char *world_leader(const wl_world *w)
{
    if (w->room == NULL || w->room->leader == NULL) return NULL;
    return w->room->leader->ch;
}

camera_view *camera_snap(wl_roomloop *rl, int32_t snap)
{
    wl_char *c = world_leader(g_world);
    if (c == NULL) return NULL;
    const wl_body *b = c->base.body;
    int32_t y = b->y + rl->lead_y;
    int32_t x = b->act_78 + b->x + rl->lead;
    camera_view *v = g_world->view;
    int32_t lim = v->map_w - v->vp_w;
    if (x < 0) x = 0;
    if (lim < x) x = lim;
    v->tgt_x = x;
    lim = v->map_h - v->vp_h;
    if (y < 0) y = 0;
    if (lim < y) y = lim;
    v->tgt_y = y;
    v->cap = 800;
    v->flag = 1;
    b = c->base.body;
    v = g_world->view;
    v->ear_rel = 0;
    v->ear_x = b->act_78 + b->x;
    v->ear_y = b->y;
    if (snap == 0) return NULL;
    v = g_world->view;
    v->cur_x = v->tgt_x;
    v->cur_y = v->tgt_y;
    v->vel_x = 0;
    v->vel_y = 0;
    return v;
}

void zones_reset(wl_zonestate *z, const wl_char *ld)
{
    if (ld == NULL) return;
    z->fired = 0;
    z->able = 0;
    z->phase = 0;
    z->rest = 0;
    z->hold = 0;
    z->cur = NULL;
    z->f04 = 0;
    z->under = NULL;
    z->f0c = NULL;
    z->tick = 0;
    z->wait = 0;
    for (uint16_t i = 0; i < 128; i++) {
        const wl_actor *a = (const wl_actor *)g_world->actors[i];
        if (a->base.alive == 0 || a->zone == 0) continue;
        uint32_t code = (uint32_t)a->base.code;
        if (code == 70440 || code == 70610 || code == 70620) {
            if (a->base.f00c == 1) continue;
        } else if (code >= 70101 && code <= 70103) {
            if (a->base.f00c == 2) continue;
        }
        const wl_body *lb = ld->base.body;
        if (a->base.body->x > lb->w + lb->x) continue;
        if (part_right(&a->base, 0) <= part_left(&ld->base, 0)) continue;
        if (char_part_top((const wl_char *)a, 0) > part_bottom(&ld->base, 0)) continue;
        if (part_bottom(&a->base, 0) > char_part_top(ld, 0)) {
            z->f04 = 0;
            z->cur = (wl_actor *)g_world->actors[i];
            return;
        }
    }
    z->f04 = 0;
    z->cur = NULL;
}

int quest_state(const wl_room *room, const uint8_t *rec, uint16_t *out)
{
    const uint8_t *t = room->f40ac;
    uint32_t n = (uint32_t)ld32(t, 0x1800);
    int32_t have = 0;
    for (uint32_t i = 0; i < n; i++)
        if (ld32(t, 12 * i) == ld32(rec, 8)) {
            have = ld32(t, 12 * i + 4);
            break;
        }
    if (have < ld32(rec, 0xc)) return 0;
    n = (uint32_t)ld32(t, 0x1800);
    for (uint32_t i = 0; i < n; i++)
        if (ld32(t, 12 * i) == ld32(rec, 0)) {
            if (ld32(t, 12 * i + 4) != 0) {
                if (out != NULL) *out = 73;
                return 2;
            }
            break;
        }
    if (out != NULL) *out = 72;
    return 1;
}

void view_record_init(uint8_t *r, wl_room *room, int32_t v)
{
    st32(r, 0x10, v);
    memset(r + 0x388, 0, 0x40);
    st32(r, 0x8, 1);
    st32(r, 0x0, 0);
    st32(r, 0x14, 0);
    st32(r, 0x394, 41);
    st32(r, 0x3c8, 0);
    st32(r, 0x20, 0);
    st32(r, 0x24, 0);
    st32(r, 0x28, 0);
    st32(r, 0x2c, 0);
    st16(r, 0x30, 255);
    st16(r, 0x32, 255);
    st16(r, 0x34, 0);
    st32(r, 0x38, 0);
    st32(r, 0x40, 0);
    st32(r, 0x1ac, 0);
    st16(r, 0x1c0, 255);
    st16(r, 0x1c2, 255);
    st32(r, 0x1b0, 0);
    st32(r, 0x1b4, 0);
    st32(r, 0x1b8, 0);
    st32(r, 0x1bc, 0);
    st16(r, 0x1c4, 0);
    st32(r, 0x1c8, 0);
    st32(r, 0x1d0, 0);
    st32(r, 0x33c, 0);
    st32(r, 0x380, 0);
    st32(r, 0x384, 0);
    stp(r, 0x4, room);
    st16(r, 0x3cc, 0);
    st32(r, 0x3d0, 0);
    for (uint32_t k = 0; k < 5; k++) st32(r, 0x3d8 + 4 * k, 0);
    if (ld32(r, 0x8) == 0) {
        st16(r, 0xc, 0);
    } else {
        uint32_t n = 0;
        const wl_room *rm = ldp(r, 0x4);
        if (g_world->f105c == 1010 || g_world->f105c == 1020) {
            const uint8_t *t = g_eng.data(0x6b7ee8u);   /* 1212-byte quest records, 0-terminated */
            if (ld32(t, 0) != 0) {
                uint16_t i = 0;
                const uint8_t *rec = t;
                do {
                    if (quest_state(rm, rec, NULL) == 1) n++;
                    i++;
                    rec = t + 1212u * i;
                } while (ld32(rec, 0) != 0);
            }
        }
        st16(r, 0xc, (uint16_t)n);
    }
    st32(r, 0x18, 0);
    st32(r, 0x3d4, ((const wl_room *)ldp(r, 0x4))->f4014);
    st16(r, 0x1c, 0);
    for (uint32_t k = 0; k < 8; k++) {
        st32(r, 0x340 + 8 * k, 0);
        st32(r, 0x344 + 8 * k, 0);
    }
}

/* 0x560900 + the error-log append + 0x406440 + exit, as 0x586010 / 0x58e330 inline them */
static void log_fatal(uint32_t msg)
{
    char *lb = (char *)g_eng.log;
    g_eng.h560900(g_eng.log);
    st32(lb, 0, 1);
    char *t = lb + 4;
    if (strlen(t) != 0) {
        strcat(t, S(0x854570u));   /* "\n" */
        strcat(t, S(msg));
    } else {
        strcpy(t, S(msg));
    }
    strcat(t, S(TRAILER));
    g_eng.log_fail_406440(g_eng.log, S(msg), S(TRAILER));
    g_eng.h5bf5db(0);
}

void room_state_save(uint8_t *t)
{
    /* `map` is one local across the loop: a +0x2b0 outside 1..3 keeps the last value (before
     * any, retail reads an uninitialized stack slot) */
    int32_t map = 0;
    for (uint16_t i = 0; i < 128; i++) {
        const wl_actor *a = (const wl_actor *)g_world->actors[i];
        if (a->base.alive == 0) continue;
        int32_t h = a->base.handle;
        if (h == 0) continue;
        int32_t kind = ld32(a, 0x2b0);
        if (kind == 3) continue;
        if (kind == 1) map = 0;
        else if (kind == 2) map = ld32(g_world->roominfo, 4);
        int32_t st = a->base.f00c;
        uint32_t n = (uint32_t)ld32(t, 0x7804);
        uint8_t *rec = NULL;
        for (uint32_t k = 0; k < n; k++)
            if (ld32(t, 0x1804 + 12 * k) == h) {
                rec = t + 0x1804 + 12 * k;
                break;
            }
        if (rec == NULL) {
            for (uint16_t k = 0; k < n; k++)
                if (ld32(t, 0x1804 + 12u * k) == 0) rec = t + 0x1804 + 12u * k;
            if (rec == NULL) {
                if (n == 0x800) log_fatal(0x859388u);   /* "The object perpetuity state area exceed..." */
                st32(t, 0x7804, (int32_t)(n + 1));
                rec = t + 0x1804 + 12 * n;
            }
            st32(rec, 0, h);
            st32(rec, 4, 0);
            st32(rec, 8, 0);
        }
        st32(rec, 4, st);
        st32(rec, 8, map);
    }
}

void map_res_free(uint8_t *m)
{
    if (ldp(m, 0x34) != NULL) {
        g_eng.op_delete(ldp(m, 0x34));
        stp(m, 0x34, NULL);
    }
    if (ldp(m, 0x38) != NULL) {
        g_eng.op_delete(ldp(m, 0x38));
        stp(m, 0x38, NULL);
    }
    if (ldp(m, 0x3c) == NULL) return;
    for (int32_t i = 0; i < ld32(m, 0x2c); i++)
        for (uint32_t k = 0; k < 4; k++) {
            void *p = ldp((uint8_t *)ldp(m, 0x3c) + 16u * (uint32_t)i, 4 * k);
            if (p != NULL) {
                g_eng.op_delete(p);
                stp((uint8_t *)ldp(m, 0x3c) + 16u * (uint32_t)i, 4 * k, NULL);
            }
        }
    if (ldp(m, 0x3c) != NULL) {
        g_eng.op_delete(ldp(m, 0x3c));
        stp(m, 0x3c, NULL);
    }
}

void *map_res_delete(uint8_t *m, int32_t flags)
{
    map_res_free(m);   /* the same frees, inlined in retail */
    if (flags & 1) g_eng.op_delete(m);
    return m;
}

void *pool_head_delete(uint8_t *h, int32_t flags)
{
    for (uint16_t i = 0; i < ld16(h, 4); i++) {
        uint8_t *r = ldp((uint8_t *)ldp(h, 0) + 4u * i, 0);
        if (r == NULL) continue;
        if (ldp(r, 0x48) != NULL) {
            for (uint16_t j = 0; j < ld16(r, 0x4c); j++) {
                void *o = ldp((uint8_t *)ldp(r, 0x48) + 4u * j, 0);
                if (o == NULL) continue;
                g_eng.h40e0c0(o);
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
    if (flags & 1) g_eng.op_delete(h);
    return h;
}

/* the view's 27 draw pools: (record offset, capacity, buffer bytes = capacity x 60), in retail's
 * allocation order (record 0 stays empty) */
static const struct { uint8_t off; uint16_t cap; uint32_t size; } POOLS[26] = {
    { 0x08, 0x080, 0x1e00 }, { 0x10, 0x1b8, 0x6720 }, { 0x18, 0x1b8, 0x6720 }, { 0x20, 0x080, 0x1e00 },
    { 0x28, 0x080, 0x1e00 }, { 0x30, 0x400, 0xf000 }, { 0x38, 0x060, 0x1680 }, { 0x40, 0x080, 0x1e00 },
    { 0x48, 0x0e0, 0x3480 }, { 0x50, 0x094, 0x22b0 }, { 0x58, 0x400, 0xf000 }, { 0x60, 0x060, 0x1680 },
    { 0x70, 0x060, 0x1680 }, { 0x68, 0x060, 0x1680 }, { 0x78, 0x080, 0x1e00 }, { 0x88, 0x400, 0xf000 },
    { 0x90, 0x080, 0x1e00 }, { 0x98, 0x1b8, 0x6720 }, { 0xa0, 0x0c8, 0x2ee0 }, { 0xa8, 0x1b8, 0x6720 },
    { 0x80, 0x040, 0x0f00 }, { 0xc0, 0x040, 0x0f00 }, { 0xb0, 0x060, 0x1680 }, { 0xb8, 0x060, 0x1680 },
    { 0xc8, 0x400, 0xf000 }, { 0xd0, 0x400, 0xf000 },
};

/* the map sprite's palette (0x586d0b..0x586f7e) */
static void map_palette(void)
{
    const uint8_t *lut = NULL;
    if (ld32(g_eng.data(0x8a9510u), 0) != 0 || ld32(g_eng.data(0x8a9514u), 0) != 1000)
        lut = g_eng.data(0x8a9410u);   /* the gamma LUT */
    if (sprite_slot_ready(ldp(g_eng.sys, 0xcd8))) return;
    const uint8_t *pd = ldp(g_eng.sys, 0x1b0c);   /* the palette descriptor */
    int32_t kind = ld32(pd, 4);
    if (kind == 0 && ld32(pd, 0xc) == 0 && ld32(pd, 8) == 0) return;
    if (kind == 3) {
        switch (ld32(g_eng.data(0x8a93fcu), 0)) {
        case 0: sprite_slot_tint(ldp(g_eng.sys, 0xcd8), 1000, 1000, 1000, lut); break;
        case 1: sprite_slot_tint(ldp(g_eng.sys, 0xcd8), 1000, 700, 600, lut); break;
        case 2: case 3: case 4: case 5: sprite_slot_tint(ldp(g_eng.sys, 0xcd8), 300, 300, 400, lut); break;
        default: break;
        }
        return;
    }
    uint8_t pal[1024], gam[1024];
    uint8_t *src = ldp(pd, 0xc);
    if (src == NULL) {
        if (ld32(pd, 8) != 0) {
            g_eng.h4178e0(ldp(g_eng.sys, 0xcd8), IP(pal));
            g_eng.h417bc0(g_eng.sys, IP(ldp(ldp(g_eng.sys, 0x1b0c), 8)), IP(pal));
        } else {
            g_eng.h4178e0(ldp(g_eng.sys, 0xcd8), IP(pal));
        }
        pd = ldp(g_eng.sys, 0x1b0c);
        src = pal;
    }
    kind = ld32(pd, 4);
    int32_t tod = ld32(g_eng.data(0x8a93fcu), 0);
    int32_t r = -1, g = 0, b = 0;
    if (kind == 2 && tod == 3) { r = 90; g = 90; b = 90; }
    else if (kind == 1 || kind == 2)
        switch (tod) {
        case 0: r = 100; g = 100; b = 100; break;
        case 1: r = 100; g = 80; b = 70; break;
        case 2: case 3: r = 70; g = 70; b = 80; break;
        case 4: r = 60; g = 60; b = 75; break;
        case 5: r = 80; g = 80; b = 85; break;
        default: break;
        }
    if (r >= 0) {
        for (int i = 0; i < 256; i++) g_eng.h4182d0(g_eng.tint, IP(pal + 4 * i), IP(src + 4 * i), r, g, b, 0);
        src = pal;
    }
    if (lut != NULL) {
        for (int i = 0; i < 256; i++)
            for (int k = 0; k < 3; k++) gam[4 * i + k] = lut[src[4 * i + k]];
        src = gam;
    }
    /* byte 3 of every entry is never written (retail passes stack garbage there, or the
     * descriptor's own); rhost's uninit-fix zeroes it at every call of the copy */
    for (int i = 0; i < 256; i++) src[4 * i + 3] = 0;
    g_eng.h491770(ldp(g_eng.sys, 0xcd8), IP(src));
}


int load_room(wl_roomloop *rl, wl_room *rooms, uint16_t nrooms, uint8_t *areas, uint8_t *rinfo, int32_t gate,
              int32_t a7, const uint8_t *keep, const uint8_t *fill, int32_t fd4, int32_t wide, int32_t f2c,
              int32_t f27b4, uint8_t *wait, int32_t f2780, int32_t f105c)
{
    step_log(0x8a2dd0u);   /* "Start StartArea" */
    void *sc = rl->scene;
    rl->scene_ev = 0;
    rl->scene_arg = 0;
    rl->scene_face = 0;
    if (sc != NULL) {
        g_eng.op_delete(sc);
        rl->scene = NULL;
    }
    rl->fd4 = fd4;
    rl->rooms = NULL;
    rl->nrooms = 0;
    rl->fec = 0;
    memset((uint8_t *)rl + 0x14, 0, 0x2c);
    rl->f2c = f2c;
    memset(rl->f54, 0, sizeof rl->f54);
    memset(rl->f90, 0, sizeof rl->f90);
    rl->lead = -30000;
    rl->lead_y = -27200;
    rl->bgm_fixed = 0;
    rl->bgm_id = 0;
    rl->bgm_wait = 0;
    memset(rl->f40, 0, sizeof rl->f40);

    /* the world */
    wl_world *old = W;
    if (old != NULL) {
        world_free(old);
        g_eng.op_delete(old);
        W = NULL;
    }
    if (rl->fe8 != NULL) {
        pool_head_delete(rl->fe8, 1);
        rl->fe8 = NULL;
    }
    void *mem = g_eng.op_new(0x27b8);
    W = mem != NULL ? world_new(mem) : NULL;
    g_world = W;
    W->f105c = f105c;
    W->f1044 = (uint8_t *)rl;
    W->areas = areas;
    W->roominfo = rinfo;
    W->scene_kind = ld32(W->roominfo, 0x114);
    W->f1034 = 0;
    W->busy = 0;
    W->f2770 = 0;
    W->f2774 = 0;
    W->f2778 = 0;
    W->f2780 = f2780;
    W->f27b4 = f27b4;
    W->f27b0 = wait;
    rl->f10 = 0;
    rl->f90c = 0;

    /* the 16-byte pool head: 32 records of 0x84 */
    uint8_t *ph = g_eng.op_new(0x10);
    if (ph != NULL) {
        st32(ph, 0, 0);
        st16(ph, 4, 0);
        st16(ph, 6, 0);
    }
    rl->fe8 = ph;
    st32(ph, 8, ld32(g_eng.sys, 0x2720));
    st32(ph, 0xc, ld32(g_eng.sys, 0x2724));
    st16(ph, 6, 0);
    st16(ph, 4, 32);
    stp(ph, 0, g_eng.op_new(0x80));
    for (uint16_t i = 0; i < ld16(ph, 4); i++) {
        uint8_t *rec = g_eng.op_new(0x84);
        if (rec != NULL) {
            st32(rec, 0x48, 0);
            st32(rec, 4, 0);
        }
        stp((uint8_t *)ldp(ph, 0) + 4u * i, 0, rec);
    }

    /* the effects-volume fader */
    uint8_t *fd = W->f1040;
    st16(fd, 4, 10);
    st16(fd, 6, 120);
    st16(fd, 8, 1200);
    stp(fd, 0, g_eng.op_new(0x3840));
    st32(fd, 0x18, 1);
    st32(fd, 0xc, 0);
    st32(fd, 0x20, 0);
    st32(fd, 0x24, 0);
    volume_fade(fd, 1);
    st32(fd, 0x28, 1000);

    /* the rooms */
    rl->rooms = rooms;
    rl->nrooms = nrooms;
    for (uint16_t i = 0; i < rl->nrooms; i++) {
        memset(rl->rooms[i].f4070, 0, sizeof rl->rooms[i].f4070);
        rl->rooms[i].f4080 = 0;
    }
    if (rl->rooms != NULL)
        for (uint16_t i = 0; i < rl->nrooms; i++)
            for (int k = 0; k < 8; k++) rl->rooms[i].slot[k]->ch = NULL;
    W->room = rl->nrooms != 0 ? rl->rooms : NULL;
    W->f27a4 = 135;

    /* the entry gate: from another map? */
    const uint8_t *ri = W->roominfo;
    for (uint16_t c = 0; c < 20; c++) {
        const uint8_t *z = ri + 12u * c + 0x1c;
        if ((uint32_t)ld16(z, 0) != (uint32_t)gate) continue;
        const uint8_t *t = W->areas;
        for (uint16_t k = 0; k < ld16(t, 0x54004); k++) {
            const uint8_t *a = t + 336u * k + 4;
            if (ld32(a, 0) != ld32(z, 4)) continue;
            if (ld32(a, 4) != ld32(ri, 4) && ld32(a, 0) != 999999) rl->f24 = 1;
            break;
        }
        break;
    }
    step_log(0x8a2dc0u);   /* "PreviousGate" */

    /* the joysticks */
    for (uint32_t k = 0; k < 2; k++) {
        uint8_t *p = (uint8_t *)g_eng.sys + 0x287c + 4 * k;
        if (ldp(p, 0) != NULL) continue;
        g_eng.h5ba120(ldp(g_eng.sys, 0x2870), IP(p), ld32(g_eng.data(0x8a93ecu), 0), (intptr_t)k);
        if (ldp(p, 0) != NULL) g_eng.h5ba290(ldp(p, 0));
    }

    /* fresh bodies for every entity band */
    rl->pcursor = 0;
    rl->fd0 = 0;
    rl->fd2 = 0;
    rl->fcc = 0;
    for (int i = 0; i < 64; i++) entity_bodies_alloc(W->band64[i], 64);
    for (int i = 0; i < 32; i++) {
        wl_char *c = W->chars[i];
        entity_bodies_alloc(&c->base, 64);
        char_links_clear(c);
    }
    for (int i = 0; i < 128; i++) entity_bodies_alloc(W->actors[i], 64);
    for (int i = 0; i < 1024; i++) entity_bodies_alloc(W->particles[i], 1);
    for (int i = 0; i < 96; i++) entity_bodies_alloc(W->band96[i], 1);
    for (int i = 0; i < 128; i++) entity_bodies_alloc(W->band2560[i], 1);
    step_log(0x8a2db0u);   /* "Init Objects" */

    /* the view's draw pools */
    view_pools_free(W->view);
    {
        uint8_t *v = (uint8_t *)W->view;
        stp(v, 0x54, g_eng.op_new(0xd8));
        st16(v, 0x58, 27);
        for (uint16_t i = 0; i < ld16(v, 0x58); i++) {
            st16((uint8_t *)ldp(v, 0x54) + 8u * i, 0, 0);
            st16((uint8_t *)ldp(v, 0x54) + 8u * i, 2, 0);
            st32((uint8_t *)ldp(v, 0x54) + 8u * i, 4, 0);
        }
    }
    for (int i = 0; i < 26; i++) {
        uint8_t *v = (uint8_t *)W->view;
        st16((uint8_t *)ldp(v, 0x54) + POOLS[i].off, 0, 0);
        st16((uint8_t *)ldp(v, 0x54) + POOLS[i].off, 2, POOLS[i].cap);
        void *buf = g_eng.op_new(POOLS[i].size);
        stp((uint8_t *)ldp(v, 0x54) + POOLS[i].off, 4, buf);
    }
    rl->view[0].f8 = 0;
    rl->view[1].f8 = 0;

    /* the room-start script (mode 7) */
    wl_event *ev = W->event;
    ev->room = W->room;
    ev->f10 = gate;
    ev->f00 = rl;
    ev->actor = NULL;
    ev->mode = 7;
    ev->unload = 0;
    if (g_eng.h40c380(ev) == 0 && ev->unload != 0) {
        void *ts = ldp(g_eng.sys, 0xaa8);
        if (ts != NULL) timed_sounds_fade(ts, 400);
        g_eng.dbg_out(S(0x854798u));   /* "Unload Resource From 'StartEvent'" */
        sys_unload(g_eng.sys, 4);
    }

    /* the map */
    void *hmod = ldp(g_eng.sys, 0x31c);
    int32_t loc = rl->fd4;
    if (W->areas != NULL && ldp(W->areas, 0x54008) != NULL) hmod = ldp(W->areas, 0x54008);
    uint8_t *mr = g_eng.op_new(0x40);
    if (mr != NULL) {
        st32(mr, 0x34, 0);
        st32(mr, 0x38, 0);
        st32(mr, 0x3c, 0);
    }
    /* the id goes in as dx over the fd4 still in edx (0x587970 reads the word) */
    uint16_t mid = ld16(rinfo, 0xc);
    if (g_eng.h587970(mr, IP(hmod), (intptr_t)(((uint32_t)loc & 0xffff0000u) | mid)) == 0) {
        char buf[256];
        g_eng.h5bf3ee(buf, S(0x8a2da8u), (int)mid);   /* "NO:%d" */
        g_eng.h426110(g_eng.log, IP(S(0x8a2d8cu)), 1, IP(S(TRAILER)));   /* "The map data is not found. " */
        g_eng.h5bf5db(0);
    }
    g_eng.h587e00(W->grid, IP(mr), ld32(rinfo, 0x110), loc, ld32(rinfo, 0x10c));
    g_eng.h58d460(rl, ld32(rinfo, 0), IP(mr));
    grid_refresh(W->grid, 1);
    map_res_free(mr);
    if (mr != NULL) map_res_delete(mr, 1);
    step_log(0x8a2d7cu);   /* "Load Map Data" */
    if (fill != NULL)
        for (int i = 0; i < 128; i++) {
            const wl_entity *a = W->actors[i];
            if (a->alive == 0 || a->handle == 0) continue;
            if (fill_lookup(fill, a->handle, &loc)) g_eng.h41e600(W->actors[i], loc);
        }

    map_palette();
    g_eng.h418470(ldp(g_eng.sys, 0xcd8), 0);
    g_eng.h417c40(g_eng.sys, 0x8c, 0);
    g_eng.h417c40(g_eng.sys, 0x8d, 0);

    /* the view */
    {
        const uint8_t *g = W->grid;
        camera_view *cv = W->view;
        cv->map_w = ld32(g, 0x2c1038);
        cv->map_h = ld32(g, 0x2c103c);
        cv->cur_x = 0;
        cv->cur_y = 0;
        st32(cv, 0x74, 0);
        cv->vp_w = 64000;
        cv->vp_h = 48000;
        cv->vel_x = 0;
        cv->vel_y = 0;
        cv->ear_rel = 1;
        cv->ear_x = 0;
        cv->ear_y = 0;
        cv->flag = 0;
        cv->tgt_x = 0;
        cv->tgt_y = 0;
        shake_clear(&cv->shake_x);
        shake_clear(&cv->shake_y);
    }
    step_log(0x8a2d68u);   /* "Setup View Manager" */

    /* the gate actor */
    wl_actor *gt = NULL;
    for (uint32_t i = 0; i < 128; i++) {
        const wl_actor *a = (const wl_actor *)W->actors[i];
        if (a->base.alive != 0 && a->zone_kind != 0 && (a->zone == gate || gate == 0)) {
            gt = (wl_actor *)W->actors[i];
            break;
        }
    }
    if (gt == NULL) {
        for (uint32_t i = 0; i < 128; i++) {
            const wl_actor *a = (const wl_actor *)W->actors[i];
            if (a->base.alive != 0 && a->zone_kind != 0) {
                gt = (wl_actor *)W->actors[i];
                break;
            }
        }
        if (gt == NULL) log_fatal(0x8a2d50u);   /* "The gate is not found. " */
    }

    /* the map NPCs present at entry.  `pct` is one local across the loop: classes 3..8 keep
     * the last one set (before any, retail reads an uninitialized stack slot; the corpus never
     * reaches this pass) */
    int32_t pct = 0;
    for (int k = 0; k < 32; k++) {
        wl_char *c = W->chars[k];
        if (c->base.alive == 0) continue;
        int32_t id = char_map_id(c);
        if (id == 0 || !char_map_npc(c)) continue;
        if (keep != NULL) {
            uint16_t n = ld16(keep, 0x84), j;
            for (j = 0; j < n; j++)
                if (ld32(keep, 4 + 4u * j) == id) break;
            if (j == n) {
                char_release(c);
                continue;
            }
        } else {
            switch (c->presence) {
            case 0: pct = 70; break;
            case 1: case 9: pct = 100; break;
            case 2: pct = 25; break;
            default: break;
            }
            int32_t roll = (int32_t)(1000u * rng_rand()) / 0x8000 % 100;
            if (roll > pct) {
                char_release(c);
                continue;
            }
        }
        if (c->presence == 9) continue;
        int near = wide != 0 ? entity_near(&gt->base, 68000, 24000, &c->base, -8000)
                             : entity_near(&gt->base, 24000, 16000, &c->base, -8000);
        if (near) char_release(c);
    }

    rl->zones.fired = 0;
    rl->zones.able = 0;
    rl->zones.phase = 0;
    rl->zones.rest = 0;
    rl->zones.hold = 0;
    rl->zones.cur = NULL;
    rl->zones.f04 = 0;
    rl->zones.under = NULL;
    rl->zones.f0c = NULL;
    rl->zones.tick = 0;
    rl->zones.wait = 0;
    step_log2(0x8a2d40u);   /* "InitAreaGate" */
    step_log2(0x8a2d28u);   /* "DetachPartyCharacter" */

    /* the party */
    for (uint16_t j = 0; j < rl->nrooms; j++) {
        for (int k = 0; k < 8; k++) {
            wl_pslot *s = rl->rooms[j].slot[k];
            if (!pslot_ok(s, 1, 0)) continue;
            pslot_status_clear(s);
            step_log2(0x8a2d10u);   /* "CreatePartyCharacter Bf" */
            wl_room *r = &rl->rooms[j];
            g_eng.h58e170(rl, IP(r), IP(gt), IP(s), a7, r->f4018);
            step_log2(0x8a2cf8u);   /* "CreatePartyCharacter Af" */
        }
        g_eng.h41dc90(&rl->rooms[j]);
        step_log2(0x8a2ce8u);   /* "UpdateFormation" */
        room_input_flush(&rl->rooms[j]);
        wl_room *r = &rl->rooms[j];
        if (r->f40a4 != 0) view_record_init((uint8_t *)rl + 0x128 + 0x3ecu * r->partner, r, r->f40a4);
    }

    /* a two-room wait record: where its actor stands */
    if (W->f27b4 == 1) {
        W->f27ac = wait;
        if (W->f27ac != NULL) {
            wl_actor *g2 = gate_actor(ld32(W->f27ac, 0x68));
            if (g2 == NULL) {
                g_eng.h426110(g_eng.log, IP(S(0x8a2d50u)), 1, IP(S(TRAILER)));
                g_eng.h5bf5db(0);
            }
            st32(W->f27ac, 0x6c, part_left(&g2->base, 0));
            st32(W->f27ac, 0x70, part_bottom(&g2->base, 0) + 1);
            st32(W->f27ac, 0x74, 0);
            st32(W->f27ac, 0x78, 0);
        }
    } else {
        W->f27ac = NULL;
    }

    camera_snap(rl, 1);
    step_log2(0x8a2cd4u);   /* "SetViewPosNormal" */
    st32(rl->f11c, 0x20, 0);
    st32(rl->f11c, 0, 0);
    st32(rl->f120, 0x20, 0);
    st32(rl->f120, 0, 0);
    st32(rl->f124, 0x20, 0);
    st32(rl->f124, 0, 0);
    rl->enc_ctr = 0;
    if (W->room != NULL) encounter_tick(rl, W->room, 1);
    step_log2(0x8a2cc8u);   /* "Start Event" */

    int res;
    int r0 = event_run(W->event, rl, W->room, NULL, ld32(rinfo, 0), gate, 0);
    if (r0 > 0 && r0 <= 3) res = r0;
    else if (r0 == 6) res = 6;
    else {
        if (W->room != NULL) zones_reset(&rl->zones, room_leader(W->room));
        step_log2(0x8a2cb8u);   /* "Start Area Loop" */
        for (;;) {
            res = room_loop_run(rl);
            *g_eng.quiet = 1;
            if (res == 1) {
                wl_room *rm = W->room;
                int r1 = event_run(W->event, rl, rm, NULL, rm->f4024, rm->f4028, 1);
                if (r1 == 11) continue;
                if (r1 == 1 || r1 == 2 || r1 == 6) res = r1;
                break;
            }
            if (res == 2 || res == 6 || res == 10) break;
            if (res == 4) {
                if (event_run(W->event, rl, W->room, NULL, 0, 0, 5) == 6) res = 6;
                break;
            }
            if (res == 5) {
                g_eng.h40a5d0(g_eng.sys, 27, 0, 0, 0, 1);
                break;
            }
        }
    }

    if (W->room != NULL) room_state_save(W->room->f40ac);
    g_eng.h56dc20(g_eng.sys, 0, 0, 0);
    if (res > 0 && res <= 2 && rl->scene != NULL && W->f27ac == NULL) {
        g_eng.h41a890(rl->scene, 10020, 0, 0, 0);
        if (rl->scene != NULL) {
            g_eng.op_delete(rl->scene);
            rl->scene = NULL;
        }
    }
    return res;
}
