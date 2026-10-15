/* script_engine.c — FUN_0040c380 + its runner helpers.  See script_engine.h.
 *
 * Transcribed from the asm: IDA's runner type (cls_401D40) names every field from +0x2e0 up 4
 * bytes low (its field_2E0 is +0x2e4, field_57C +0x580), so the runner offsets here are the
 * asm's.  The chapter scripts are called by VA (g_eng.script_call: PORT-DEBT(engine-call-slots)). */
#include "script_engine.h"

#include <stddef.h>
#include <string.h>

#include "char_ai.h"
#include "char_integrate.h"
#include "char_state.h"
#include "engine_calls.h"
#include "entity_life.h"
#include "gimmick.h"
#include "particle_spawn.h"
#include "room_loop.h"
#include "sound_fx.h"

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
static void *zdm(void) { return ldp(g_eng.sys, 0x2884); }
static wl_roomloop *RL(const wl_event *ev) { return (wl_roomloop *)ev->f00; }

/* the chapter scripts, in retail's order */
static const uint32_t AREA_1010_EARLY[] = {   /* flag 100100101 in 0..500 */
    0x4d7d80u, 0x4dc510u, 0x4d64f0u, 0x4cd6d0u, 0x4d13a0u, 0x4ccc90u, 0x4c6b40u, 0x4e19e0u,
};
static const uint32_t AREA_1010_LATE[] = {    /* flag 100100101 in 1000..2900 */
    0x4fec40u, 0x4fa2a0u, 0x4f6a10u, 0x5034b0u, 0x510100u, 0x507e70u, 0x525ae0u, 0x4e68d0u, 0x4ef4e0u,
    0x4f19e0u, 0x4e6a60u, 0x4ee690u, 0x4ea7b0u, 0x515a00u, 0x51a1d0u, 0x522310u, 0x523010u, 0x523300u,
    0x530080u, 0x525e70u, 0x533580u, 0x535520u, 0x536480u, 0x537440u, 0x539e80u, 0x53ec40u, 0x5402e0u,
    0x546670u,
};
static const uint32_t AREA_ANY[] = {          /* after the empty 0x4afda0 */
    0x49d930u, 0x4a0c90u, 0x49f7c0u, 0x4a6b80u, 0x4ac6d0u, 0x4a66a0u, 0x4a5fa0u, 0x4ae150u, 0x4ae420u,
    0x4afdb0u, 0x4b3b20u, 0x4b3c70u, 0x4b4d40u, 0x4bbef0u, 0x4bd2e0u, 0x4bd6b0u, 0x4bd820u, 0x4bdf90u,
    0x4bdbe0u, 0x4bdbe0u, 0x4be220u, 0x4c1a30u, 0x4c1c20u, 0x4c2310u, 0x4c29c0u, 0x4c2dc0u, 0x4c3ee0u,
    0x4c45a0u,
};
#define N(a) (sizeof(a) / sizeof((a)[0]))

/* 1 = stop with *r (a script handled the event, or set unload: then 0) */
static int scripts(wl_event *ev, const uint32_t *va, size_t n, int *r)
{
    for (size_t i = 0; i < n; i++) {
        *r = g_eng.script_call(va[i], ev);
        if (*r != 0) return 1;
        if (ev->unload != 0) return 1;
    }
    return 0;
}

int script_none(void) { return 0; }

int script_wait(wl_event *ev) { return g_eng.h439690(ev->f00, IP(ev)); }

int32_t script_flag(const wl_event *ev, int32_t id)
{
    const uint8_t *t = ev->room->f40ac;
    uint32_t n = (uint32_t)ld32(t, 0x1800);
    for (uint32_t i = 0; i < n; i++)
        if (ld32(t, 12 * i) == id) return ld32(t, 12 * i + 4);
    return 0;
}

wl_char *script_leader(const wl_event *ev)
{
    const wl_pslot *ld = ev->room->leader;
    return ld != NULL ? ld->ch : NULL;
}

void *actor_by_record(int32_t id)
{
    const wl_list *l = W->list_2790;
    for (uint16_t i = 0; i < l->count; i++)
        if (ld32(l->items[i], 0x1d8) == id) return l->items[i];
    return NULL;
}

void script_cast_add(wl_event *ev, void *actor)
{
    if (actor == NULL) return;
    uint8_t *r = (uint8_t *)ev, *rec = NULL;
    uint16_t k;
    for (k = 0; k < 32; k++)
        if (ldp(r, 0x304 + 20u * k) == actor && ld32(r, 0x2fc + 20u * k) != 0) break;
    if (k == 32)
        for (k = 0; k < 32 && ld32(r, 0x2fc + 20u * k) != 0; k++) {}
    if (k < 32) {
        rec = r + 0x2fc + 20u * k;
        st32(rec, 0, 1);
        st32(rec, 4, 1);
        memcpy(rec + 8, &actor, 4);
        memcpy(rec + 0xc, &actor, 4);
        st32(rec, 0x10, 0);
    }
    /* (all 32 taken: retail reads the record at NULL — a crash) */
    uint8_t *a = (uint8_t *)ldp(rec != NULL ? rec : (uint8_t *)(volatile uintptr_t)0, 0xc);
    st32(a, 0x15b8c, 0);
    st16(a, 0x15b68, 0);
    st16(a, 0x15b6a, 0);
    st32(a, 0x15b90, 0);
    st32(a, 0x15ba8, 0);
    st16(a, 0x1429c, 0);
    st16(a, 0x147a0, 0);
    st32(a, 0x15b64, 3);
    st32(r, 0x20, 4);
}

void script_party_cast(wl_event *ev)
{
    for (int k = 0; k < 8; k++) {
        const wl_pslot *s = ev->room->slot[k];
        if (s->active == 1) script_cast_add(ev, actor_by_record(s->id));
    }
}

void script_begin(wl_event *ev)
{
    uint8_t *r = (uint8_t *)ev;
    st32(r, 0x24, 0); st32(r, 0x78, 0); st32(r, 0x6c, 0); st32(r, 0x2e4, 0); st32(r, 0x74, 0);
    st32(r, 0x30, 0); st32(r, 0x84, 0); st32(r, 0x2ec, 0); st16(r, 0x584, 0); st32(r, 0xb28, 0);
    ev->unload = 1;
    st32(r, 0x57c, 0); st32(r, 0x580, 0); st32(r, 0xb20, 0); st32(r, 0xb24, 0); st32(r, 0x20, 0);
    st32(r, 0x1c, 0);
    wl_char *ld = script_leader(ev);
    memcpy(r + 0x60, &ld, 4);
    st32(r, 0x64, 0); st32(r, 0x68, 0); st32(r, 0x40, 0); st32(r, 0x4c, 0); st32(r, 0x5c, 0);
    st16(r, 0x2f0, 0);
    for (int k = 0; k < 32; k++) {
        st32(r, 0x2fc + 20u * k, 0);
        st32(r, 0x304 + 20u * k, 0);
        st32(r, 0x308 + 20u * k, 0);
    }
    st32(r, 0x18, 0);
    W->f2760 = 1;
    uint8_t *rl = (uint8_t *)ev->f00;
    memset(rl + 0x54, 0, 60);
    memset(rl + 0x90, 0, 60);
    for (int i = 0; i < 32; i++) {
        uint8_t *c = (uint8_t *)W->chars[i];
        if (ld32(c, 0x1d0) == 0) continue;
        wl_body *b = ((wl_char *)c)->base.body;
        st32(c, 0x14858, 0); st32(c, 0x14854, 0); st32(c, 0x1485c, 0); st32(c, 0x14860, 0);
        st16(c, 0x14868, 0); st32(c, 0x14864, 0); st16(c, 0x1486a, 0); st32(c, 0x1486c, 0);
        st16(c, 0x14870, 0); st16(c, 0x1422c, 0); st16(c, 0x1422e, 0); st16(c, 0x14230, 0);
        st16(c, 0x1429c, 0); st16(c, 0x14278, 0); st16(c, 0x147ac, 0); st16(c, 0x147f4, 0);
        st16(c, 0x14232, 0); st16(c, 0x14802, 0); st32(c, 0x14270, 0); st32(c, 0x1427c, 0);
        st16(c, 0x14234, 0); st16(c, 0x14236, 0); st16(c, 0x14804, 0); st16(c, 0x14288, 0);
        st32(c, 0x1428c, 0); st16(c, 0x14248, 0); st16(c, 0x14290, 0); st32(c, 0x14294, 0);
        st32(c, 0x1423c, 0); st32(c, 0x14810, 0); st32(c, 0x14814, 0); st32(c, 0x268, 1);
        st32(c, 0x15b64, 0); st32(c, 0x15b8c, 0); st16(c, 0x15b68, 0); st16(c, 0x15b6a, 0);
        st32(c, 0x15b90, 0); st32(c, 0x15ba8, 0); st16(c, 0x147a0, 0);
        if (b->state == 2 || b->state == 5) { b->state = 0; b->sub = 0; b->ctr = 0; }
        st16(c, 0x158a8, 0);
    }
}

void script_end(wl_event *ev)
{
    wl_roomloop *rl = RL(ev);
    uint8_t *p = (uint8_t *)rl;
    if (zdm() != NULL) g_eng.h5bcfb0(zdm(), 0, 50, 0);
    for (uint32_t off = 0x54; off <= 0x90; off += 0x3c) {
        uint8_t *o = (uint8_t *)ldp(p, off);
        if (o != NULL) st32(o, 0x50, 0);
        memset(p + off, 0, 60);
    }
    st16(p, 0x30, 0);
    rl->hitstop = 0;
    rl->hit_ph = 0;
    rl->hit_n = 0;
    W->f2760 = 0;
    W->busy = 0;
    rl->f40[0] = 0;
    for (int v = 0; v < 2; v++)
        st16(&rl->view[v], 0xc, rl->view[v].f8 != 0 ? (uint16_t)g_eng.h49ce70((void *)(intptr_t)rl->view[v].f4) : 0);
    for (int i = 0; i < 32; i++) {
        wl_char *c = W->chars[i];
        if (c->base.alive == 0) continue;
        cmd_clear((wl_cmd *)((uint8_t *)c + 0x14854));
        char_ai_reset(c, 0);
        c->f268 = 0;
        c->f15b5c = 0;
    }
    st32(ev, 0x2e4, 1);
}

/* the scene command block for a plain wait (mode 0 / 1) */
static void wait_setup(wl_event *ev, int32_t f20, int32_t f28, int32_t f34, int32_t f3c, int32_t f38)
{
    st32(ev, 0x30, 1);
    st32(ev, 0x34, f34);
    st32(ev, 0x3c, f3c);
    st32(ev, 0x38, f38);
    st32(ev, 0x24, 1);
    st32(ev, 0x28, f28);
    st32(ev, 0x20, f20);
    st32(ev, 0x2c, 1000);
}

/* the first free message-log slot (world +4 / +8, 8 x 516 B from world+0x1024) gets `text` */
static void log_line(const char *text)
{
    uint8_t *w = (uint8_t *)W;
    int32_t i = ld32(w, 0x1024);
    for (uint16_t n = 0; ld32(w, 516 * i + 4) != 0; ) {
        i = (i + 1) % 8;
        if (++n >= 8) return;
    }
    st32(w, 516 * i + 4, 1);
    strcpy((char *)w + 516 * i + 8, text);
}

/* mode 0: the room arrival */
static int arrival(wl_event *ev)
{
    script_begin(ev);
    wl_roomloop *rl = RL(ev);
    if (W->room != NULL && rl->f2c != 0) {
        if (rl->f2c == 1) log_line(S(0x8547f8u));        /* "You have teleported to a safe place." */
        else if (rl->f2c == 2) log_line(S(0x854820u));   /* "The teleport trap has deposited you ..." */
        uint16_t k = 0;
        for (int i = 0; i < 8; i++) {
            const wl_pslot *s = W->room->slot[i];
            if (s->active != 1 || s->ch == NULL) continue;
            uint8_t *c = (uint8_t *)s->ch;
            st16(c, 0x158a8, 2);
            st32(c, 0x158ac, 40);
            st32(c, 0x158b0, 40);
            st32(c, 0x158b4, 0);
            int32_t t = 30 * k + 40;
            st32(c, 0x158b8, t < 1 ? 1 : t);
            k++;
        }
    }
    rl->bgm_fixed = 0;
    rl->bgm_id = 0;
    uint16_t id = bgm_pick(rl);
    if (zdm() != NULL && ld32(cfg(), 0x174) != 0
        && (g_eng.h5bbc60(zdm(), id) == 0 || g_eng.h5bbc90(zdm(), id) != 0)) {
        st16(g_eng.sys, 0x2888, id);
        g_eng.h5bbcd0(zdm(), 20);
        g_eng.h5bbc20(zdm());
        if (g_eng.h5bcb80(zdm(), id, 1, 0, 0, 0) == 0)
            g_eng.h408b90(g_eng.log, IP((uint8_t *)zdm() + 0x2c), 0, IP(S(0x8a9b6cu)));
    }
    wait_setup(ev, 2, 1, 2, 500, 1);
    int r = script_wait(ev);
    if (r == 6) return 6;
    st32(ev, 0x1c, 0);
    if (rl->f24 != 0) {
        const uint8_t *t = (const uint8_t *)ldp(W->areas, 0);
        int32_t map = ld32(W->roominfo, 4);
        uint16_t k = 0;
        for (int32_t id2 = ld32(t, 0); id2 != 0; id2 = ld32(t, 64u * ++k))
            if (id2 == map) {
                const char *name = (const char *)t + 64u * k + 4;
                if (name[0] != 0) {
                    uint8_t *rec = (uint8_t *)rl->f11c;
                    void *obj = ldp(rec, 0xc);
                    st32(rec, 0, 1);
                    st32(rec, 0x20, 1);
                    st16(rec, 4, 0);
                    st16(rec, 6, 0);
                    st32(rec, 8, 0);
                    st32(rec, 0x1c, 0);
                    g_eng.h4031c0(obj, IP(name), 0);
                    st16(rec, 0x10, 200);
                }
                break;
            }
    }
    rl->f24 = 0;
    script_end(ev);
    return r;
}

/* mode 1: a door */
static int door(wl_event *ev)
{
    unsigned k;
    for (k = 0; k < 128; k++) {
        const wl_entity *a = W->actors[k];
        if (a->alive != 0 && ld32(a, 0x278) != 0 && (ld32(a, 0x274) == ev->f10 || ev->f10 == 0)) break;
    }
    if (k < 128 && W->actors[k] != NULL && ld32(W->actors[k], 0xc) == 1) {
        script_begin(ev);
        g_eng.h49d6e0(ev, 3, 0, IP(S(0x8547e4u)), 0, 1, 0, 0, 0, 0, 0);   /* "The door is locked." */
        int r = script_wait(ev);
        if (r == 6) return 6;
        script_end(ev);
        return 11;
    }
    script_begin(ev);
    st32(ev, 0x580, 1);
    st32(ev, 0x2e4, 1);
    wait_setup(ev, 2, 2, 3, 1000, 0);
    int r = script_wait(ev);
    if (r == 6) return 6;
    st32(ev, 0x1c, 1);
    script_end(ev);
    return r;
}

/* the opened furniture's first idle state record (+0x208, 12 B {state, clip, u16 sound}; u16 +0x268
 * of them): `call` = 1 sets them through 0x407b80 / 0x407ba0 (the talk path), 0 inline (the chest) */
static void actor_settle(wl_entity *a, wl_body *b, int call)
{
    uint16_t n = ld16(a, 0x268);
    st32(a, 0xc, 0);
    uint16_t k;
    for (k = 0; k < n && ld32(a, 0x208 + 12u * k) != 0; k++) {}
    if (k >= n) return;
    const uint8_t *rec = (const uint8_t *)a + 0x208 + 12u * k;
    const void *clip = ldp(rec, 4);
    uint16_t snd = ld16(rec, 8);
    if (clip != NULL) body_set_clip(b, clip);
    if (snd == 0) return;
    if (call) { sfx_play_at(a, 0, snd, 0, 0, 0); return; }
    wl_voice *v = vpool_take(*(wl_vpool **)((uint8_t *)g_eng.sys + 0x364 + 4u * snd), 0);
    if (v == NULL) return;
    void *buf = v->buf;
    int32_t vol, pan;
    if (sfx_place(a, ld32(cfg(), 0x168), 0, 0, b, &vol, &pan) == 0) return;
    g_eng.dsb_volume_5bb870(buf, vol);
    g_eng.dsb_pan_5bb880(buf, pan);
    g_eng.dsb_freq_5bb890(buf, 0);
    g_eng.dsb_play_5bb8c0(buf);
}

/* the opened state by code (0 = skip the settle) */
static int actor_opened(wl_entity *a, wl_body *b, int talk)
{
    int32_t code = (int32_t)a->code;
    if (code == 70820) { entity_set_collide(a, 3, 3); return 1; }
    if (code == 70230) return !(ld32(a, 0x28c) == 1 && ld32(a, 0xc) == 1);
    if (code == 70251 || code == 70252 || code == 70371) {
        if (talk) switch_glow((wl_actor *)a, 0, 1);
        else if (particle_find(a, 0) == NULL)
            particle_spawn(a, a, ld32(a, 0x2e4) == 1 ? 100135 : 100134, b, 0, 0, 99, 1, 0, 0, 2, 1, 0, 20);
    }
    return 1;
}

/* mode 3, 70100: the save statue (the save menu 0x408730, then the room BGM restarted) */
static int save_statue(wl_event *ev)
{
    script_begin(ev);
    int r = g_eng.h408730(ev, 1);
    wl_roomloop *rl = RL(ev);
    rl->bgm_fixed = 0;
    rl->bgm_id = 0;
    g_eng.h40a5d0(g_eng.sys, bgm_pick(rl), 1, 1, 20, 0);   /* (0x40d9b0 inlined) */
    script_end(ev);
    return r;
}

/* mode 3: an actor switched (furniture, the chest, a talk record) */
static int switched(wl_event *ev, int r)
{
    wl_entity *a = (wl_entity *)ev->actor;
    int32_t rec = ld32(a, 0x1d8);
    if (rec == 310140502 || rec == 210140513 || rec == 270190501 || rec == 300250501 || rec == 340165501
        || rec == 360160501) {
        script_begin(ev);
        wl_char *ld = script_leader(ev);
        st32(ev, 0x4c, 0); st32(ev, 0x50, 0); st32(ev, 0x54, 0);
        memcpy((uint8_t *)ev + 0x5c, &ld, 4);
        st32(ev, 0x58, 800);
        st32(ev, 0x20, 3);
        script_party_cast(ev);
        if ((r = script_wait(ev)) == 6) return 6;
        if (g_eng.h402d40(ev, 0, IP(ev->room)) != 0 && (r = script_wait(ev)) == 6) return 6;
        g_eng.h406210(ev, 0, IP(ev->actor), 0, 0);
        if ((r = script_wait(ev)) == 6) return 6;
        ld = script_leader(ev);
        memcpy((uint8_t *)ev + 0x5c, &ld, 4);
        st32(ev, 0x4c, 0); st32(ev, 0x50, 0); st32(ev, 0x54, 0);
        st32(ev, 0x58, 800);
        st32(ev, 0x20, 3);
        if ((r = script_wait(ev)) == 6) return 6;
        if (g_eng.h411610(ev) == 6) return 6;
        if (ld32(a, 0xc) != 0 && actor_opened(a, a->body, 1)) actor_settle(a, a->body, 1);
        script_end(ev);
        return r;
    }
    switch ((int32_t)a->code) {
    case 70100:
        return save_statue(ev);
    case 70220: {
        script_begin(ev);
        st32(ev, 0x57c, 10);
        st32(ev, 0x20, 6);
        if ((r = script_wait(ev)) == 6) return 6;
        int32_t opened;
        r = g_eng.h4081d0(ev, IP(a), ld32(a, 0x2cc), ld32(a, 0x2d0), IP(&opened), IP(S(0x8547d4u)), 1);
        script_end(ev);   /* "treasure chest" */
        if (opened == 0 || ld32(a, 0xc) == 0) return r;
        wl_body *b = a->body;
        if (actor_opened(a, b, 0)) actor_settle(a, b, 0);
        return r; }
    case 70302: case 70303: case 70304:
        return g_eng.h407970(ev, IP(a), 0);
    case 70353: case 70358:
        return g_eng.h407970(ev, IP(a), IP(S(0x8547ccu)));   /* "closet" */
    case 70357: case 70364: case 70366:
        return g_eng.h407970(ev, IP(a), IP(S(0x8547c4u)));   /* "shelf" */
    case 70363: case 70365:
        return g_eng.h407970(ev, IP(a), IP(S(0x8547bcu)));   /* "drawer" */
    default:
        return r;
    }
}

int script_engine(wl_event *ev)
{
    int r;
    switch (W->f105c) {
    case 3:
        if (scripts(ev, (const uint32_t[]){ 0x4e1d10u }, 1, &r)) return r;
        break;
    case 1010:
        if (script_flag(ev, 100100101) >= 0 && script_flag(ev, 100100101) <= 500
            && scripts(ev, AREA_1010_EARLY, N(AREA_1010_EARLY), &r)) return r;
        if (script_flag(ev, 100100101) < 1000 || script_flag(ev, 100100101) > 2900) break;
        if (scripts(ev, AREA_1010_LATE, N(AREA_1010_LATE), &r)) return r;
        break;
    case 1020:
        if (scripts(ev, (const uint32_t[]){ 0x4e23d0u }, 1, &r)) return r;
        break;
    default:
        break;
    }
    if ((r = script_none()) != 0) return r;
    if (ev->unload != 0) return 0;
    if (scripts(ev, AREA_ANY, N(AREA_ANY), &r)) return r;
    switch (ev->mode) {
    case 0: return arrival(ev);
    case 1: return door(ev);
    case 3: return switched(ev, r);
    case 5: return g_eng.h40a340(ev);
    case 7: {
        int32_t t = script_flag(ev, 100100102);
        RL(ev)->fd4 = (t == 10 || t == 30 || t == 40 || t == 50 || t == 60) ? script_flag(ev, 100100102) : 20;
        return r; }
    default:
        return r;
    }
}
