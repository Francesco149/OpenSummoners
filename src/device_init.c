/* device_init.c — FUN_00431e30 + its helpers.  See device_init.h.
 *
 * Transcribed from the asm (the decompile hides the u16 stores at +0x2a0 / +0x2ec / +0x2f4 /
 * +0x1ec).  MSVC inlined the entity setup 0x426620 into 18 of the cases; each inline is the same
 * code (kind 2, tint 9, 4 effect records), so every case calls entity_setup. */
#include "device_init.h"

#include <stddef.h>
#include <string.h>

#include "char_integrate.h"
#include "char_physics.h"
#include "char_state.h"
#include "engine_calls.h"
#include "entity_life.h"
#include "gimmick.h"
#include "npc_ai.h"
#include "actor_ops.h"
#include "actor_update.h"
#include "particle_init.h"
#include "particle_spawn.h"
#include "world_alloc.h"

static int32_t ld32(const void *p, uint32_t off) { int32_t v; memcpy(&v, (const uint8_t *)p + off, 4); return v; }
static void st32(void *p, uint32_t off, int32_t v) { memcpy((uint8_t *)p + off, &v, 4); }
static void st16(void *p, uint32_t off, uint16_t v) { memcpy((uint8_t *)p + off, &v, 2); }

void entity_snd_free(wl_entity *e)
{
    if (e->snd == NULL) return;
    for (uint16_t i = 0; i < e->nsnd; i++) {
        /* (retail logs "ReserveWave over index." for i >= the count: unreachable inside the loop) */
        if (e->snd == NULL) continue;
        wl_sndch *c = &e->snd[i];
        if (c->voice == NULL) continue;
        g_eng.dsb_stop_5bb8e0(c->voice->buf);
        c->voice->owner = 0;
        c->voice = NULL;
        c->pool = NULL;
        c->f20 = 0;
    }
    if (e->snd != NULL) {
        g_eng.op_delete(e->snd);
        e->snd = NULL;
    }
}

void device_step_add(wl_actor *a, int32_t state, const void *clip, uint16_t sfx)
{
    if (a->nsteps >= 8) return;
    a->steps[a->nsteps].state = state;
    a->steps[a->nsteps].clip = clip;
    a->steps[a->nsteps].sfx = sfx;
    a->nsteps++;
}

void device_tint(wl_actor *a, int32_t t)
{
    a->base.f0fc = t;
    a->base.f100 = t;
}

void device_zone(wl_actor *a, int32_t kind, int32_t id)
{
    if (id != 0) {
        a->zone = id;
        a->zone_kind = kind;
    } else {
        a->zone_kind = 0;
        a->zone = 0;
    }
}

int32_t device_link_state(const wl_actor *a, int32_t st)
{
    if (a->link == 0) return st;
    const wl_entity *l = actor_by_handle(a->link);
    if (l == NULL) return 0;
    return l->f00c != 0 ? a->step == 1 : a->step != 1;
}

void device_night_glow(wl_actor *a, int32_t dx, int32_t dy)
{
    int32_t tod = *g_eng.tod;
    if (tod == 2 || tod == 3) particle_spawn(&a->base, &a->base, 100125, a->base.body, dx, dy, 99, 1, 0, 0, 2, 1, 0, 20);
}

void grid_mark_d(uint8_t *grid, int32_t x, int32_t y, int32_t w, int32_t h, uint16_t v)
{
    int32_t d0 = ld32(grid, 0x2c1030), d1 = ld32(grid, 0x2c1034);
    for (int32_t i = 0; i < h; i += 3200)
        for (int32_t j = 0; j < w; j += 3200) {
            int32_t row = (y + i) / 3200, col = (x + j) / 3200;
            if (col >= 0 && col < d0 && row >= 0 && row < d1) st16(grid, 0x2c1040u + 2u * (uint32_t)(col * 128 + row), v);
        }
}

void device_parts_copy(wl_actor *a)
{
    wl_entity *e = &a->base;
    for (uint16_t k = 1; k < e->nbody; k++) {
        uint8_t *b = (uint8_t *)&e->body[k];
        const uint8_t *p = b - sizeof(wl_body);
        b[0x00] = p[0x00];
        memcpy(b + 0x04, p + 0x04, 16);   /* x y w h */
        st32(b, 0x14, 0);
        memcpy(b + 0x18, p + 0x18, 4);
        st32(b, 0x1c, 0);
        b[0x20] = p[0x20];
        memcpy(b + 0x24, p + 0x24, 20);   /* +0x24 .. +0x34 */
        memcpy(b + 0x38, p + 0x38, 6);    /* state sub ctr */
        st32(b, 0x40, e->dst_x);
        st32(b, 0x44, e->dst_y);
        memcpy(b + 0x48, p + 0x48, 2);
        memcpy(b + 0x4c, p + 0x4c, 8);
        memcpy(b + 0x54, p + 0x54, 8);    /* four u16 */
        memcpy(b + 0x5c, p + 0x5c, 8);
        memcpy(b + 0x64, p + 0x64, 4);    /* two u16 */
        memcpy(b + 0x6c, p + 0x6c, 4);    /* the clip */
        memcpy(b + 0x70, p + 0x70, 4);    /* timer frame */
        b[0x74] = p[0x74];
        memcpy(b + 0x78, p + 0x78, 4);
        b[0x7c] = b[0x7d] = b[0x7e] = b[0x7f] = 0;
        st16(b, 0x80, 0);
        memcpy(b + 0x284, p + 0x284, 12);
        if (e->rects == NULL) continue;
        for (uint16_t r = 0; r < e->nrects; r++) {
            uint8_t *fr = (uint8_t *)e->body[k].frames + 20u * r;
            memcpy(fr, e->rects + 32u * r, 16);
            st32(fr, 0x10, 0);
        }
    }
}

wl_actor *device_spawn(wl_entity *owner, uint32_t code, const wl_body *b, int32_t dx, int32_t dy, int32_t facing,
                       int32_t anchor, int32_t none, int32_t anchor2)
{
    if (none != 0) return NULL;
    wl_actor *a = NULL;
    for (uint16_t i = 0; i < 128; i++)
        if (g_world->actors[i]->alive == 0) {
            a = (wl_actor *)g_world->actors[i];
            break;
        }
    if (a == NULL) return NULL;
    int32_t x, y;
    switch (anchor) {
    case 1: x = b->w / 2 + b->x + dx; y = b->h / 2 + b->y + dy; break;
    case 2: x = b->x + dx; y = b->margin + b->y + dy; break;
    case 3: x = b->w / 2 + b->x + dx; y = b->margin + b->y + dy; break;
    case 4: x = b->w / 2 + b->x + dx; y = b->h + b->y + dy; break;
    default: x = dx; y = dy; break;
    }
    device_init(a, x, y, anchor2, code, 0, 0, 0, 0, 0, 0, facing, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                ld32(g_world->roominfo, 0), owner);
    return a;
}

int wl_list_push(wl_list *l, wl_entity *e)
{
    if (l->count >= l->cap) return 0;
    l->items[l->count++] = e;
    return 1;
}

/* the fatal unknown code */
static void device_fatal(void)
{
    g_eng.h426110(g_eng.log, (intptr_t)g_eng.data(0x855274u), 1, (intptr_t)g_eng.data(0x8a9b6cu));
    g_eng.h5bf5db(0);
}

void device_init(wl_actor *a, int32_t x, int32_t y, int32_t anchor, uint32_t code, int32_t kind6, int32_t trk,
                 int32_t handle, int32_t zone, int32_t state, int32_t kind, int32_t facing, int32_t x0, int32_t x1,
                 int32_t y0, int32_t y1, int32_t p0, int32_t p1, int32_t p2, int32_t p3, int32_t gx, int32_t gy,
                 int32_t link, int32_t ride, int32_t map, wl_entity *follow)
{
    wl_entity *e = &a->base;
    wl_body *b0 = e->body;   /* (retail keeps part 0 from entry) */
    int32_t serial = 0, solid = 0;
    entity_snd_free(e);
    e->nsnd = 1;
    e->snd = g_eng.op_new(0x24);
    memset(e->snd, 0, 36u * e->nsnd);
    a->zone_kind = 0;
    a->zone = 0;
    a->area_x0 = x0;
    a->area_x1 = x1;
    a->area_y0 = y0;
    a->area_y1 = y1;
    a->nsteps = 0;
    a->cooldown = 0;
    a->use_mover = 0;
    a->link = link;
    a->goal_x = gx;
    a->goal_y = gy;
    st32(a, 0x2a4, 0);
    a->multipart = 0;
    a->f284 = 0;
    a->kind = kind;
    a->invert = 0;
    st32(a, 0x2b0, 1);
    a->step = p0;
    a->f2d0 = p1;
    a->f2d4 = p2;
    st32(a, 0x2d8, p3);
    a->material = 0;
    a->hit = 0;
    st32(a, 0x2dc, 0);
    a->f2e4 = kind6;
    st32(a, 0x2f0, 0);
    st16(a, 0x2f4, 0);
    a->f2e8 = 0;
    st16(a, 0x2ec, 0);
    a->hold = 0;
    a->link_all = 0;
    a->follow = follow;
    a->f2f8 = 0;
    if (a->f2a8 != NULL) {
        g_eng.op_delete(a->f2a8);
        a->f2a8 = NULL;
    }
    a->f1f8 = NULL;
    /* the 8-byte rows of 0x693808 (0-terminated): the room record's +0x140 picks one, else row 0 */
    {
        const uint8_t *t = g_eng.data(0x693808u);
        const void *row = NULL;
        if (ld32(t, 0) != 0) {
            int32_t key = ld32(g_world->roominfo, 0x140);
            for (uint16_t i = 0; ld32(t, 8u * i) != 0; i++)
                if (ld32(t, 8u * i) == key) {
                    row = t + 8u * i;
                    break;
                }
        }
        a->f2e0 = row != NULL ? row : (const int32_t *)t;
    }

#define SETUP(w, h, dx, dy) entity_setup(e, 2, (int32_t)code, 0, x, y, (w), (h), facing, 1, (dx), (dy), anchor, 9, 4)
#define TRK(id, bb) entity_track_on(e, 0, (int16_t)(id), (int16_t)(bb))
#define TRKF(id, bb) entity_set_track(e, 0, (int16_t)(id), (int16_t)(bb), 1, 0, 0, 0)
#define COLL(f, m, k) do { e->f1e0 = (f); entity_set_collide(e, (m), (k)); } while (0)
#define STEPS(c0, s0, c1, s1) do { device_step_add(a, 0, (c0), (s0)); device_step_add(a, 1, (c1), (s1)); } while (0)
#define P19 do { if (p2 != 0) { if (p2 == 2) st32(a, 0x2b0, 3); } else st32(a, 0x2b0, 2); } while (0)
#define AREA_SHIFT do { a->area_x0 += -100 * e->dst_x; a->area_y0 += -100 * e->dst_y; \
                        a->area_x1 += -100 * e->dst_x; a->area_y1 += -100 * e->dst_y; } while (0)
#define D(va) g_eng.data(va)
    switch (code) {
    /* ── 70100..70105 ── */
    case 70100:
        SETUP(8000, 8000, 0, 0);
        TRKF(436, 0);
        COLL(0, 0, 0);
        body_set_clip(b0, D(0x6477f8u));
        STEPS(NULL, 0, NULL, 0);
        break;
    case 70101:
        entity_setup(e, 2, (int32_t)code, 0, x, y - 12800, 3200, 16000, facing, 1, 0, 0, anchor, 9, 4);
        a->zone_kind = zone != 0;
        a->zone = zone;
        break;
    case 70102: case 70103: case 70105: {
        if (code == 70105) SETUP(3200, 3200, 0, 0);
        else SETUP(6400, 3200, 0, 0);
        int32_t k = code == 70102 ? 2 : code == 70103 ? 3 : 4;
        a->zone_kind = zone != 0 ? k : 0;
        a->zone = zone;
        break;
    }
    case 70120:
        a->grow_max = y1 - y0;
        a->grow_min = y1 - y;
        entity_setup(e, 2, 70120, 0, x0, y0, x1 - x0, a->grow_max, facing, 1, 0, 0, anchor, 9, 4);
        STEPS(NULL, 0, NULL, 0);
        P19;
        if (a->step == 0) a->step = 400;
        break;
    /* ── 70130..70367 ── */
    case 70130:
        if (y0 <= 0) y0 = -64000;
        entity_setup(e, 2, (int32_t)code, 0, x0, y0, x1 - x0, y1 - y0 + 3200, facing, 1, 0, 0, anchor, 9, 4);
        STEPS(NULL, 0, NULL, 0);
        st32(a, 0x2b0, 3);
        break;
    case 70201:
        SETUP(6400, 1600, 0, 0);
        TRK(342, 0);
        COLL(3, 3, 2);
        STEPS(NULL, 0, NULL, 0);
        P19;
        a->multipart = 1;
        break;
    case 70202:
        if (anchor == 1) y -= 6400;
        SETUP(9600, 1600, 0, -64);
        TRK(348, 1);
        COLL(3, 3, 2);
        STEPS(NULL, 0, NULL, 0);
        P19;
        a->multipart = 1;
        device_tint(a, 9);
        e->f100 = 7;
        break;
    case 70205:
        SETUP(6400, 6400, 0, 0);
        TRK(364, 3);
        COLL(3, 3, 3);
        if (a->kind == 2) {
            e->f1c4 = 100;
            e->f1c8 = a->area_x0;
            e->f1cc = a->area_x1;
            a->use_mover = 1;
        }
        a->material = 1;
        a->multipart = 1;
        break;
    case 70210:
        SETUP(6400, 6400, 0, 0);
        TRK(342, 1);
        COLL(5, 3, 3);
        if (a->kind == 2) {
            e->f1c4 = 100;
            e->f1c8 = a->area_x0;
            e->f1cc = a->area_x1;
            a->use_mover = 1;
        } else if (a->kind == 4) {
            a->use_mover = 1;
        }
        device_tint(a, 18);
        a->multipart = 1;
        e->weight = 4;
        break;
    case 70215:
        SETUP(4800, 800, -8, -64);
        TRK(342, a->kind == 2 ? 38 : 8);
        COLL(5, 3, 3);
        STEPS(D(0x671918u), 225, D(0x671ab0u), 225);
        if (p0 == 1) a->invert = 1;
        if (p2 == 0) st32(a, 0x2b0, 2);
        else if (p2 == 2) st32(a, 0x2b0, 3);
        solid = 3;
        break;
    case 70216:
    case 70217:
        SETUP(3200, 3200, -16, -16);
        TRK(342, code == 70216 ? 12 : 14);
        COLL(0, 0, 0);
        device_step_add(a, 0, D(0x671918u), code == 70216 && a->kind == 2 ? 0 : 225);
        device_step_add(a, 1, D(0x671ab0u), 225);
        if (p0 == 1) a->invert = 1;
        if (p2 == 0) st32(a, 0x2b0, 2);
        else if (p2 == 2) st32(a, 0x2b0, 3);
        device_tint(a, 5);
        break;
    case 70220:
        SETUP(4800, 3200, -16, -32);
        TRK(342, 4);
        STEPS(D(0x671918u), 237, D(0x671ab0u), 237);
        break;
    case 70230:
        SETUP(2200, 12800, -22, 0);
        TRK(347, 0);
        entity_set_collide(e, 3, 3);
        a->multipart = 1;
        P19;
        break;
    case 70240:
        SETUP(5600, 2400, -4, 0);
        TRK(342, 16);
        COLL(0, 0, 0);
        e->faction = 2;
        a->f2e8 = 1;
        break;
    case 70245: case 70246: case 70247: case 70248:
        if (code == 70245) { SETUP(4800, 3200, -8, -32); TRK(342, 18); }
        else if (code == 70246) { SETUP(4800, 3200, -8, 0); TRK(342, 19); }
        else if (code == 70247) { SETUP(3200, 4800, 0, -8); TRK(342, 26); }
        else { SETUP(3200, 4800, -32, -8); TRK(342, 27); }
        COLL(0, 0, 0);
        AREA_SHIFT;
        e->faction = 2;
        a->f2e8 = 1;
        serial = 1;
        a->multipart = 1;
        P19;
        break;
    case 70251:
    case 70252:
    case 70371:
        if (code == 70251) { SETUP(6400, 6400, 0, 32); TRK(342, trk); }
        else if (code == 70252) { SETUP(6400, 6400, 0, 24); TRK(347, trk); }
        else { SETUP(6400, 6400, 0, 0); TRK(0, 0); }
        COLL(0, 0, 0);
        if (a->kind != 1) a->material = 3;
        STEPS(NULL, 82, NULL, 239);
        P19;
        break;
    case 70260:
        SETUP(6400, 6400, 0, 0);
        TRK(364, trk);
        COLL(0, 0, 0);
        device_night_glow(a, 0, 1600);
        break;
    case 70265:
        SETUP(4000, 4000, 0, 0);
        TRK(363, trk);
        COLL(0, 0, 0);
        device_night_glow(a, 0, 0);
        break;
    case 70270:
        SETUP(2400, 4000, -20, -24);
        TRK(342, 21);
        COLL(5, 3, 2);
        solid = 3;
        break;
    case 70280: case 70281: case 70282: case 70290:
        if (code == 70290) {
            SETUP(12800, 3200, 0, -64);
            TRK(kind6 == 1 ? 362 : 361, trk);
            COLL(5, 3, 3);
        } else {
            SETUP(12800, 1600, 0, -64);
            TRK(359, trk);
            COLL(3, 3, 2);
        }
        device_tint(a, 18);
        e->f100 = 9;
        STEPS(NULL, 0, NULL, 0);
        grid_mark_d(g_world->grid, b0->x, b0->y, b0->w, 100, 4);
        solid = 3;
        break;
    case 70285:
        SETUP(12800, 1600, 0, -32);
        TRK(360, trk);
        COLL(0, 0, 0);
        device_tint(a, 18);
        break;
    case 70302:
    case 70304:
        SETUP(2800, 4800, -18, -16);
        TRK(364, code == 70302 ? 1 : 6);
        COLL(0, 0, 0);
        a->material = code == 70302 ? 1 : 2;
        break;
    case 70303:
        SETUP(4800, 4800, -16, -16);
        TRK(364, 2);
        COLL(3, 3, 2);
        a->material = 1;
        solid = 3;
        break;
    case 70305:
    case 70306:
        SETUP(3200, 6400, -16, 0);
        TRK(364, trk);
        COLL(0, 0, 0);
        device_zone(a, 4, zone);
        break;
    case 70310:
        SETUP(3200, 3200, -16, -32);
        TRKF(364, trk);
        COLL(0, 0, 0);
        a->zone_kind = zone != 0 ? 4 : 0;
        a->zone = zone;
        break;
    case 70311:
        SETUP(6400, 6400, 0, 0);
        TRKF(364, trk);
        COLL(0, 0, 0);
        e->f0fc = 5;
        e->f100 = 5;
        break;
    case 70351: SETUP(10000, 4800, -14, -80); TRK(367, trk); COLL(0, 0, 0); break;
    case 70354: SETUP(8000, 4800, -24, -80); TRK(367, trk); COLL(0, 0, 0); break;
    case 70356: SETUP(7200, 11500, -28, -13); TRK(368, trk); COLL(0, 0, 0); break;
    case 70368: SETUP(6400, 6400, 0, 0); TRK(365, trk); COLL(0, 0, 0); break;
    case 70352:
    case 70355:
        if (code == 70352) SETUP(9600, 3200, -16, -96);
        else SETUP(8800, 2800, -20, -100);
        TRK(367, trk);
        COLL(0, 3, 2);
        solid = 3;
        device_zone(a, 4, zone);
        break;
    case 70353: case 70357: case 70358: case 70359: case 70363: case 70364: case 70365: case 70366:
        switch (code) {
        case 70353: SETUP(7200, 11500, -28, -13); TRK(367, trk); break;
        case 70357: SETUP(8000, 8000, 0, -48); TRK(367, trk); break;
        case 70358: SETUP(9600, 16000, -16, -32); TRK(369, trk); break;
        case 70359: SETUP(12800, 8000, -32, -112); TRK(369, trk); break;
        case 70363: case 70364: SETUP(6400, 6400, 0, 0); TRK(364, trk); break;
        default: SETUP(6400, 6400, 0, 0); TRK(365, trk); break;
        }
        COLL(0, 0, 0);
        device_tint(a, 5);
        break;
    case 70360:
        SETUP(12800, 12800, 0, 0);
        TRK(367, trk);
        COLL(0, 0, 0);
        particle_spawn(e, e, 100099, b0, 5400, 6200, 99, 2, 0, 0, 0, 0, 0, 20);
        break;
    case 70361:
        SETUP(4000, 4000, 0, 0);
        TRK(363, trk);
        COLL(0, 0, 0);
        body_set_clip(b0, D(0x647990u));
        body_clip_jitter(b0);
        e->f24 = 6400;
        break;
    case 70362:
        if (sys_by_mode(g_eng.sys, 1, 1, 1) != 0) return;
        SETUP(4000, 4000, 0, 0);
        TRK(363, trk);
        COLL(0, 0, 0);
        body_set_clip(b0, D(0x647b28u));
        body_clip_jitter(b0);
        break;
    case 70367:
        SETUP(2400, 5200, -20, -12);
        TRK(364, trk);
        COLL(5, 3, 2);
        break;
    case 70370:
    case 70501: case 70502: case 70503:
    case 70511: case 70512: case 70513: case 70514: case 70515: case 70516:
    case 70520:
    case 70375:
        SETUP(3200, 3200, 0, 0);
        break;
    case 70521: {
        int32_t w = a->area_x1 - a->area_x0;
        if (w < 3200) w = 3200;
        SETUP(w, 3200, 0, 0);
        break;
    }
    case 70372:
        SETUP(6400, 6400, 0, 0);
        TRK(419, 0);
        COLL(0, 0, 0);
        body_set_clip(b0, D(0x647e58u));
        body_clip_jitter(b0);
        e->tint_f4 = ld32(g_eng.sys, 0x2790);
        e->tint_f8 = ld32(g_eng.sys, 0x2790);
        device_tint(a, 6);
        break;
    case 70373:
        SETUP(6400, 6400, 0, 0);
        TRK(364, trk);
        COLL(0, 0, 0);
        device_tint(a, 10);
        STEPS(NULL, 0, NULL, 0);
        break;
    case 70374:
        SETUP(3200, 3200, 0, 0);
        device_night_glow(a, 0, 0);
        break;
    /* ── 70380..70620 ── */
    case 70380:
        SETUP(16000, 3600, -16, -156);
        TRK(369, trk);
        COLL(0, 3, 2);
        solid = 3;
        device_zone(a, 4, zone);
        break;
    case 70390:
        SETUP(3200, 3200, -240, -240);
        TRK(399, 0);
        COLL(0, 0, 0);
        STEPS(NULL, 0, NULL, 0);
        break;
    case 70400:
        SETUP(4000, 4000, -12, -12);
        TRK(342, trk);
        COLL(1, 3, 3);
        a->material = 4;
        solid = 3;
        break;
    case 70410:
    case 70411: {
        int32_t w = 32, h = 40, dx = -48, dy = -120, id = 345;   /* 70410 */
        if (code == 70411) {
            /* (a kind outside 0..2 reads two stale stack slots in retail) */
            int32_t w0 = 0, h0 = 0;
            if (kind6 == 0) { w0 = 30; h0 = 44; }
            else if (kind6 == 1) { w0 = 30; h0 = 35; }
            else if (kind6 == 2) { w0 = 30; h0 = 28; }
            w = w0; h = h0; dx = (80 - w0) / -2; dy = h0 - 80; id = 346;
        }
        SETUP(100 * w, 100 * h, dx, dy);
        TRK(id, trk);
        if (a->kind == 2) {
            COLL(5, 3, 3);
            solid = 3;
            AREA_SHIFT;
            e->f1c4 = 100;
            e->f1c8 = a->area_x0;
            e->f1cc = a->area_x1;
            a->use_mover = 1;
            a->multipart = 1;
        } else if (a->kind == 3) {
            COLL(0, 0, 0);
        } else {
            if (code == 70410) COLL(5, 3, 2);
            else COLL(5, 3, 3);
            solid = 3;
        }
        break;
    }
    case 70420: case 70460: case 70470:
        if (code == 70420) {
            if (anchor == 1) y -= 400;
            SETUP(6400, 800, 0, -4);
            TRK(342, 24);
            COLL(4, 3, 2);
        } else if (code == 70460) {
            if (anchor == 1) y -= 8000;
            SETUP(9600, 1600, 0, -80);
            TRK(348, 2);
            COLL(3, 3, 2);
        } else {
            if (anchor == 1) y -= 6400;
            SETUP(9600, 9600, 0, -64);
            TRK(348, 3);
            COLL(5, 3, 3);
            e->weight = 3;
        }
        if (a->area_y0 == a->area_y1) a->area_y1 = ld32(g_world->grid, 0x2c103c) + 12800;
        e->f100 = 7;
        STEPS(NULL, 0, NULL, 0);
        P19;
        a->multipart = 1;
        if (a->kind == 3 && a->area_y0 == b0->y) b0->y = (a->area_y1 + a->area_y0) / 2;
        break;
    case 70430:
        SETUP(12000, 1600, 4, 0);
        TRK(348, 0);
        COLL(3, 3, 2);
        e->faction = 2;
        serial = 1;
        a->multipart = 1;
        P19;
        break;
    case 70440:
        SETUP(6400, 3200, -64, -128);
        TRK(350, 0);
        COLL(0, 0, 0);
        P19;
        STEPS(NULL, 0, NULL, 0);
        a->area_y0 = b0->y;
        device_zone(a, 3, zone);
        break;
    case 70450:
        SETUP(2800, 4800, -18, -16);
        TRK(365, 4);
        COLL(0, 0, 0);
        STEPS(D(0x671918u), 0, D(0x671ab0u), 0);
        break;
    case 70480:
        SETUP(8800, 4000, -4, -56);
        TRK(349, 0);
        COLL(3, 3, 3);
        e->f1c4 = 100;
        e->f1c8 = a->area_x0;
        e->f1cc = a->area_x1;
        a->use_mover = 1;
        e->f100 = 7;
        a->f1f8 = device_spawn(e, 100144, b0, 0, a->area_y0 - b0->y, 99, 2, 0, 0);
        a->multipart = 1;
        break;
    case 70600:
        SETUP(22400, 3200, -16, -16);
        TRK(343, 0);
        COLL(3, 3, 3);
        e->f1c4 = 100;
        a->multipart = 1;
        device_tint(a, 9);
        e->f100 = 7;
        P19;
        e->no_entity_coll = 1;
        e->f24 = 3200;
        break;
    case 70610:
    case 70620:
        if (code == 70610) { SETUP(6400, 12800, -64, 0); TRK(106, 0); }
        else { SETUP(6400, 16000, -64, 0); TRK(126, 0); }
        COLL(0, 0, 0);
        device_tint(a, 5);
        device_zone(a, 3, zone);
        STEPS(NULL, 243, NULL, 243);
        break;
    case 70700:
        SETUP(3200, 3200, -16, -16);
        TRK(342, trk);
        COLL(0, 0, 0);
        e->faction = 2;
        a->f2e8 = 1;
        serial = 1;
        a->multipart = 1;
        STEPS(NULL, 0, NULL, 0);
        P19;
        break;
    /* ── 70800..71031 ── */
    case 70800:
        SETUP(3200, 3200, 0, 0);
        if (a->area_y0 <= 0) a->area_y0 = -64000;
        a->area_x1 += 3200;
        a->area_y1 += 3200;
        if (p0 == 1) a->invert = 1;
        STEPS(NULL, 0, NULL, 0);
        P19;
        if (a->f2d0 > 0) a->f2d0 += 1000 * map;
        break;
    case 70810:
        SETUP(3200, 3200, 0, 0);
        TRK(344, trk);
        COLL(0, 0, 0);
        device_tint(a, 5);
        STEPS(NULL, 244, NULL, 244);
        state = device_link_state(a, state);
        break;
    case 70820: {
        if (a->area_y0 <= 0) a->area_y0 = -64000;
        a->area_y1 += 3200;
        entity_setup(e, 2, (int32_t)code, 0, a->area_x0, a->area_y0, a->area_x1 - a->area_x0,
                     a->area_y1 - a->area_y0, facing, 1, 0, 0, anchor, 9, 4);
        STEPS(NULL, 0, NULL, 0);
        break;
    }
    case 70830:
        SETUP(3200, 3200, 0, 0);
        a->link_all = 1;
        STEPS(NULL, 0, NULL, 0);
        if (a->step > 0) a->step += 1000 * map;
        if (a->f2d0 > 0) a->f2d0 += 1000 * map;
        st32(a, 0x2b0, 3);
        a->invert = 1;
        break;
    case 70840:
        SETUP(3200, 3200, 0, 0);
        break;
    case 70850:
        SETUP(3200, 3200, 0, 0);
        STEPS(NULL, 0, NULL, 0);
        st32(a, 0x2b0, 3);
        break;
    case 70910:
        SETUP(6400, 6400, 0, 0);
        TRK(113, trk);
        COLL(5, 3, 3);
        solid = 3;
        a->kind = 2;
        e->f1c4 = 100;
        a->use_mover = 1;
        a->multipart = 1;
        a->material = 2;
        break;
    case 71010:
        if (zone != 0) {
            SETUP(6400, 12800, -32, 0);
            TRKF(103, zone != 1);
            COLL(0, 0, 0);
            e->f0fc = 5;
            e->f100 = 5;
            a->zone_kind = 3;
            a->zone = zone;
        } else {
            SETUP(3200, 3200, 0, 0);
        }
        break;
    case 71031:
        SETUP(3200, 3200, 0, 0);
        break;
    /* ── 100138..100145 ── */
    case 100138:
        SETUP(1200, 1200, -10, -10);
        TRKF(435, 32);
        COLL(0, 0, 0);
        particle_spawn(e, e, 100127, b0, 0, 0, 99, 1, 0, 0, 2, 1, 0, 20);
        break;
    case 100139:
        SETUP(3200, 3200, 0, 0);
        TRKF(435, 33);
        COLL(0, 0, 0);
        break;
    case 100140:
        SETUP(4000, 4000, 0, 0);
        TRKF(435, 34);
        COLL(0, 0, 0);
        break;
    case 100141:
        SETUP(12800, 19200, 0, 0);
        TRKF(373, 0);
        COLL(0, 0, 0);
        body_set_clip(b0, D(0x671c48u));
        break;
    case 100142:
        SETUP(8000, 9600, 0, 0);
        TRKF(432, 0);
        COLL(0, 0, 0);
        body_set_clip(b0, D(0x671de0u));
        e->tint_f4 = ld32(g_eng.sys, 0x2758);
        e->tint_f8 = ld32(g_eng.sys, 0x2758);
        break;
    case 100143: {
        uint8_t *blk = g_eng.op_new(0x54);
        if (blk != NULL) char_block_init(blk);
        a->f2a8 = blk;
        SETUP(3200, 3200, 0, 0);
        break;
    }
    case 100144:
        SETUP(8800, 800, -4, -48);
        TRK(349, 8);
        COLL(3, 3, 2);
        e->f100 = 7;
        a->multipart = 1;
        break;
    case 100145:
        SETUP(3200, 3200, 0, 0);
        break;
    default:
        device_fatal();
        return;
    }
#undef SETUP
#undef TRK
#undef TRKF
#undef COLL
#undef STEPS
#undef P19
#undef AREA_SHIFT
#undef D
    if (ride != 0) e->ride_mode = ride;
    else if (solid != 0) e->ride_mode = solid;
    actor_state_restore(a, state);
    entity_set_handle(e, handle);
    if (serial) {
        wl_list_push(g_world->list_2788, e);
        st32(a, 0x2a4, (int32_t)world_next_serial(g_world));
    }
    if (a->use_mover != 0) wl_list_push(g_world->list_2794, e);
    if (a->material != 0) wl_list_push(g_world->list_2798, e);
    if (a->multipart == 0) device_parts_copy(a);
    if (a->follow != NULL) {
        a->follow_dx = b0->x - part_left(a->follow, 0);
        a->follow_dy = b0->y - char_part_top((const wl_char *)a->follow, 0);
    }
}
