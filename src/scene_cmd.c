/* scene_cmd.c — the scene command builders.  See scene_cmd.h.
 *
 * Transcribed from the asm (the runner's offsets from +0x2e0 up are 4 bytes past IDA's names, quirk
 * #144).  The cast-record search every cast command inlines is script_cast_rec. */
#include "scene_cmd.h"

#include <stddef.h>
#include <string.h>

#include "char_init.h"
#include "engine_calls.h"
#include "room_loop.h"
#include "scene_loop.h"
#include "script_engine.h"

#define IP(p) ((intptr_t)(p))
#define W (*g_eng.world)

static int32_t  ld32(const void *p, uint32_t off) { int32_t v; memcpy(&v, (const uint8_t *)p + off, 4); return v; }
static uint16_t ld16(const void *p, uint32_t off) { uint16_t v; memcpy(&v, (const uint8_t *)p + off, 2); return v; }
static void st32(void *p, uint32_t off, int32_t v) { memcpy((uint8_t *)p + off, &v, 4); }
static void st16(void *p, uint32_t off, uint16_t v) { memcpy((uint8_t *)p + off, &v, 2); }
static void *ldp(const void *p, uint32_t off) { void *v; memcpy(&v, (const uint8_t *)p + off, sizeof v); return v; }
static const char *S(uint32_t va) { return (const char *)g_eng.data(va); }

static const uint8_t *cfg(void)
{
    return (const uint8_t *)(intptr_t)ld32((const void *)(intptr_t)ld32(g_eng.sys, 0x320), 0);
}

/* "GetHeadLeftRight State Count Over." for an actor without parts (logged, not fatal) */
static void parts_check(const wl_entity *a)
{
    if (a->nbody != 0) return;
    char buf[256];
    g_eng.h5bf3ee(buf, S(0x854540u), (int)a->code);   /* "%d" */
    g_eng.log_fail_406440(g_eng.log, S(0x85451cu), buf);
}

void scene_msg(wl_event *ev, uint32_t kind, void *actor, const char *text, int32_t a5, int32_t a6, int32_t own_name,
               const char *name, int32_t voice, int32_t a10, uint16_t face)
{
    (void)a6;   /* retail never reads it */
    uint8_t *r = (uint8_t *)ev;
    const wl_entity *a = actor;
    st32(r, 0x80, a10);
    st32(r, 0x2dc, a5);
    st32(r, 0x7c, (int32_t)kind);
    memcpy(r + 0x2e0, &actor, 4);
    st16(r, 0x2e8, 0);
    strcpy((char *)r + 0x8a, text);
    int32_t rec = 0, alt = 0, dir = 1;
    if (ldp(g_eng.sys, 0xaa8) != NULL && ld32(cfg(), 0x1ac) != 0) st16(r, 0x2e8, face);
    if ((int32_t)kind >= 0 && (int32_t)kind <= 2) {
        if (a == NULL) return;
        rec = ld32(a, 0x1d8);
        parts_check(a);
        st32(r, 0x80, ld32(a, 0x1e8));
        dir = a->body->facing;
    }
    if (name != NULL) strcpy((char *)r + 0x28a, name);
    else {
        strcpy((char *)r + 0x28a, S(0x8a9b6cu));
        if (own_name != 0 && a != NULL) strcpy((char *)r + 0x28a, (const char *)ldp(a, 0x750));
    }
    st32(r, 0x78, 1);
    if (ld16(r, 0x2f0) != 0) {
        int32_t k = ld32(r, 0x7c);
        if (k >= 0 && (k <= 2 || k == 4)) {
            st32(r, 0x2e0, 0);
            alt = 1;
        }
    }
    if (voice != 32) {
        st32(r, 0x84, 0);
        if (voice != 0) {
            const uint8_t *t = (const uint8_t *)g_eng.data(0x6b6568u);
            if (ld32(t, 0) != 0) {
                uint16_t k = 0;
                while (!(ld32(t, 16u * k) == rec && ld32(t, 16u * k + 4) == voice)) {
                    k++;
                    if (ld32(t, 16u * k) == 0) { st32(r, 0x20, 1); return; }
                }
                const uint8_t *row = t + 16u * k;
                st32(r, 0x84, ld16(row, alt ? 0xc : dir == 3 ? 8 : 0xa));
                st16(r, 0x88, 0);
            }
        }
    }
    st32(r, 0x20, 1);
}

void scene_msg_rec(wl_event *ev, uint32_t kind, int32_t rec, const char *text, int32_t a5, int32_t a6,
                   int32_t own_name, const char *name, int32_t voice, int32_t a10, uint16_t face)
{
    scene_msg(ev, kind, actor_by_record(rec), text, a5, a6, own_name, name, voice, a10, face);
}

void scene_face(wl_event *ev, void *actor, int32_t dir)
{
    if (actor == NULL) return;
    uint8_t *a = script_cast_actor(script_cast_rec(ev, actor, 1));
    script_cast_cmd(a, 2);
    st32(a, 0x15b74, dir);
    st32(ev, 0x20, 4);
}

void scene_face_rec(wl_event *ev, int32_t rec, int32_t dir) { scene_face(ev, actor_by_record(rec), dir); }

int scene_party_stance(wl_event *ev, int32_t draw, wl_room *room)
{
    int any = 0;
    for (int k = 0; k < 8; k++) {
        const wl_pslot *s = room->slot[k];
        if (s->active != 1) continue;
        wl_char *c = s->ch;
        int32_t code = (int32_t)c->base.code;
        if (code != 50010 && code != 100165 && code != 100167 && code != 100168) continue;
        int32_t kind, drawn = ld32(c->party, 0xd4);
        if (drawn == 0) { if (draw == 0) continue; kind = 3; }
        else if (drawn == 1) { if (draw != 0) continue; kind = 4; }
        else continue;
        uint8_t *rec = script_cast_rec(ev, c, 1);
        uint8_t *a = (uint8_t *)s->ch;
        /* (no free record: retail writes +0xc of NULL — an access violation) */
        volatile uint8_t *rp = rec;
        memcpy((uint8_t *)rp + 0xc, &a, 4);
        script_cast_cmd(a, 4);
        st32(a, 0x15b98, kind);
        st32(a, 0x15b9c, 1);
        st32(a, 0x15ba0, 0);
        st32(a, 0x15ba4, 0);
        st32(a, 0x15b88, 100);
        any = 1;
    }
    if (any) st32(ev, 0x20, 4);
    return any;
}

int scene_talk_face(wl_event *ev, int32_t a2, void *target, int32_t face_back, int32_t step)
{
    const wl_char *ld = ev->room->leader->ch;
    int32_t dir, back = 0, to = 0;
    if (target != NULL) {
        const wl_body *lb = ld->base.body, *tb = ((const wl_entity *)target)->body;
        int32_t dx = tb->x + tb->w / 2 - lb->x - lb->w / 2;
        to = tb->w;
        if (dx > 0) {
            dir = 1;
            if (step != 0 && (dx < 0 ? -dx : dx) < 3200) { back = 1; to = tb->x - lb->w - 2800; }
        } else {
            dir = 3;
            if (step != 0 && (dx < 0 ? -dx : dx) < 3200) { back = 1; to = tb->x + tb->w + 2800; }
        }
        if (face_back != 0) {
            uint8_t *a = script_cast_actor(script_cast_rec(ev, target, 1));
            script_cast_cmd(a, 2);
            st32(a, 0x15b74, dir != 3 ? 3 : 1);
        }
    } else {
        parts_check(&ld->base);
        dir = ld->base.body->facing;
    }
    return g_eng.h405e80(ev, a2, dir, 0, back, to);   /* (`to` unread unless `back`) */
}

void script_flag_set(wl_event *ev, int32_t id, int32_t v, int32_t v2)
{
    uint8_t *t = ev->room->f40ac;
    uint8_t *f = flag_find(t, id);
    if (f == NULL) f = flag_add(t, id);
    if (v2 != 0 || v != 0) {
        st32(f, 4, v);
        st32(f, 8, v2);
    } else {
        st32(f, 0, 0);
    }
}

int32_t scene_camera(wl_event *ev, int32_t target, int32_t a3, int32_t a4, int32_t speed)
{
    st32(ev, 0x5c, target);
    st32(ev, 0x50, a3);
    st32(ev, 0x4c, 0);
    st32(ev, 0x54, a4);
    st32(ev, 0x58, speed);
    st32(ev, 0x20, 3);
    return a4;
}

void scene_next_event(wl_event *ev, int32_t ev_id, int32_t arg, int32_t face)
{
    wl_roomloop *rl = (wl_roomloop *)ev->f00;
    rl->scene_ev = ev_id;
    rl->scene_arg = arg;
    rl->scene_face = face;
    st32(ev, 0x580, 0);
}

int32_t scene_pause(wl_event *ev, int32_t n)
{
    st32(ev, 0x20, 6);
    st32(ev, 0x57c, n);
    return n;
}

void scene_mark(wl_event *ev) { st32(ev, 0x2e4, 1); }

int32_t scene_fade_out(wl_event *ev, int32_t t)
{
    st32(ev, 0x30, 1); st32(ev, 0x34, 3); st32(ev, 0x3c, t); st32(ev, 0x38, 0);
    return t;
}

int32_t scene_fade_in(wl_event *ev, int32_t t)
{
    st32(ev, 0x30, 1); st32(ev, 0x34, 2); st32(ev, 0x3c, t); st32(ev, 0x38, 1);
    return 1;
}

int32_t scene_wait(wl_event *ev, int32_t t)
{
    st32(ev, 0x20, 2); st32(ev, 0x24, 1); st32(ev, 0x28, 1); st32(ev, 0x2c, t);
    return t;
}

int32_t scene_wait_fade(wl_event *ev, int32_t t)
{
    st32(ev, 0x24, 1); st32(ev, 0x28, 2); st32(ev, 0x20, 2); st32(ev, 0x2c, t);
    return t;
}

void scene_title_show(wl_roomloop *rl, uint16_t dur)
{
    const uint8_t *t = (const uint8_t *)ldp(W->areas, 0);
    int32_t map = ld32(W->roominfo, 4);
    uint16_t k = 0;
    for (int32_t id = ld32(t, 0); id != 0; id = ld32(t, 64u * ++k)) {
        if (id != map) continue;
        const char *name = (const char *)t + 64u * k + 4;
        if (name[0] == 0) return;
        uint8_t *rec = (uint8_t *)rl->f11c;
        void *node = ldp(rec, 0xc);
        st32(rec, 0, 1);
        st32(rec, 0x20, 1);
        st16(rec, 4, 0);
        st16(rec, 6, 0);
        st32(rec, 8, 0);
        st32(rec, 0x1c, 0);
        g_eng.h4031c0(node, IP(name), 0);
        st16(rec, 0x10, dur);
        return;
    }
}

void scene_area_title(wl_event *ev, uint16_t dur)
{
    wl_roomloop *rl = (wl_roomloop *)ev->f00;
    scene_title_show(rl, dur);
    rl->f24 = 0;
    st32(ev, 0x20, 9);
}

/* the first live door actor (+0x278) with +0x274 == `door` (any when `any0` and door == 0) */
static wl_entity *door_actor(int32_t door, int any0)
{
    for (unsigned k = 0; k < 128; k++) {
        wl_entity *a = W->actors[k];
        if (a->alive != 0 && ld32(a, 0x278) != 0 && (ld32(a, 0x274) == door || (any0 && door == 0)))
            return W->actors[k];
    }
    return NULL;
}

void scene_place_at_door(wl_event *ev, void *actor, int32_t door, int32_t dx, int32_t dy, int32_t facing)
{
    if (ev->f00 == NULL || actor == NULL) return;
    wl_entity *a = actor;
    if (door != 0) {
        const wl_entity *d = door_actor(door, 0);
        if (d != NULL) {
            const wl_body *db = d->body;
            a->body->x = db->x + dx;
            a->body->y = db->y + db->h + dy - ld32(a, 0x1f4) - ld32(a, 0x2c);
        }
    }
    if (facing != 99) a->body->facing = facing;
}

void scene_place_rec(wl_event *ev, int32_t rec, int32_t door, int32_t dx, int32_t dy, int32_t facing)
{
    scene_place_at_door(ev, actor_by_record(rec), door, dx, dy, facing);
}

void room_bgm_restart(wl_roomloop *rl, int32_t a2, int32_t a3)
{
    rl->bgm_fixed = 0;
    rl->bgm_id = 0;
    g_eng.h40a5d0(g_eng.sys, bgm_pick(rl), 1, a2, 20, a3);   /* (0x40d9b0 inlined) */
}

wl_char *scene_spawn_at_door(wl_event *ev, int32_t handle, int32_t code, int32_t door, int32_t dx, int32_t dy,
                             int32_t facing, int32_t a8, int32_t a9)
{
    wl_roomloop *rl = (wl_roomloop *)ev->f00;
    if (rl == NULL) return NULL;
    wl_char *c = NULL;
    for (uint16_t n = 0; ; ) {
        c = W->chars[rl->fcc];
        rl->fcc = (uint16_t)((rl->fcc + 1) & 0x1f);
        if (c->base.alive == 0) break;
        if (++n >= 32) return NULL;
    }
    if (c == NULL) return NULL;
    const wl_entity *d = door_actor(door, 1);
    if (d == NULL) return NULL;
    const wl_body *db = d->body;
    char_init(c, db->x + dx, db->y + db->h + dy, 3, 3, 0, code, 0, facing, handle, 0, a9, a8, 0, 0);
    return c;
}
