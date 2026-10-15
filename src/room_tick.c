/* room_tick.c — FUN_0059c750 / FUN_0059aa40 / FUN_0059ca90 and helpers.  See room_tick.h.
 *
 * Transcribed from the asm.  The message-log appends retail inlines (0x44e6c0's loop) go through
 * world_log_line; GetTickCount is g_eng.tick. */
#include "room_tick.h"

#include <stddef.h>
#include <string.h>

#include "actor_update.h"
#include "char_state.h"
#include "char_status.h"
#include "engine_calls.h"
#include "gimmick.h"
#include "particle_spawn.h"
#include "party_member.h"
#include "rng.h"
#include "room_load.h"
#include "scene_loop.h"
#include "sound_fx.h"

#define W (*g_eng.world)

static int32_t  ld32(const void *p, uint32_t off) { int32_t v; memcpy(&v, (const uint8_t *)p + off, 4); return v; }
static uint16_t ld16(const void *p, uint32_t off) { uint16_t v; memcpy(&v, (const uint8_t *)p + off, 2); return v; }
static void st32(void *p, uint32_t off, int32_t v) { memcpy((uint8_t *)p + off, &v, 4); }
static void st16(void *p, uint32_t off, uint16_t v) { memcpy((uint8_t *)p + off, &v, 2); }
static void *ldp(const void *p, uint32_t off) { void *v; memcpy(&v, (const uint8_t *)p + off, sizeof v); return v; }
static const char *S(uint32_t va) { return (const char *)g_eng.data(va); }
static int32_t mul(int32_t a, int32_t b) { return (int32_t)((uint32_t)a * (uint32_t)b); }

static const uint8_t *cfg(void)
{
    return (const uint8_t *)(intptr_t)ld32((const void *)(intptr_t)ld32(g_eng.sys, 0x320), 0);
}

int room_warp_tick(wl_roomloop *rl, wl_room *room)
{
    if (rl->ui_lock != 0) {
        rl->ui_lock--;
        if (rl->ui_lock == 40) {   /* the dissolve: world+0x1040's tile grid reset */
            uint8_t *fx = W->f1040;
            st32(fx, 0xc, 2);
            int32_t r = rng_rand();
            st16(fx, 0xa, ld16(fx, 8));
            st32(fx, 0x10, mul(r, 3) / 0x8000);
            st16(fx, 0x14, 0);
            st32(fx, 0x18, 0);
            st32(fx, 0x1c, 1000);
            for (uint16_t x = 0; x < ld16(fx, 4); x++)
                for (uint16_t y = 0; y < ld16(fx, 6); y++) {
                    uint8_t *cell = (uint8_t *)ldp(fx, 0) + 12u * (x + (uint32_t)y * ld16(fx, 4));
                    st16(cell, 0, 0);
                    st16(cell, 2, 0);
                    st32(cell, 4, x);
                    st32(cell, 8, y);
                }
        }
        return rl->ui_lock != 0 ? 0 : 10;
    }
    if (rl->f10 == 0) return 0;
    rl->f10 = 0;
    if (rl->f90c == 2) {
        world_log_line(W, S(0x8a3dbcu));   /* "You triggered a teleport trap!" */
    } else if (W->f1034 != 0 || W->scene_kind == 2 || W->f2768 != 0 || ld32(room, 0x4080) != 0
               || ld32(room, 0x40d0) == 0) {
        sfx_play(g_eng.sys, 6, 0, 0);
        world_log_line(W, S(0x8a3ddcu));   /* "For some reason, you cannot teleport here." */
        return 0;
    }
    rl->ui_lock = 160;
    uint16_t k = 0;
    for (int i = 0; i < 8; i++) {
        const wl_pslot *s = room->slot[i];
        if (s->active != 1 || s->ch == NULL) continue;
        wl_char *c = s->ch;
        if (c->base.alive != 1 || c->base.body->active != 1) continue;
        st16(c, 0x158a8, 1);
        st32(c, 0x158ac, 0);
        st32(c, 0x158b0, 40);
        st32(c, 0x158b4, 0);
        int32_t t = 10 * (k + 2);
        st32(c, 0x158b8, t < 1 ? 1 : t);
        k++;
    }
    return 0;
}

int char_on_ground(wl_char *c, uint16_t part)
{
    return entity_ground_probe(&c->base, &c->base.body[part], part);
}

int char_lead_ok(wl_char *c, uint16_t part)
{
    int32_t mm = c->move_mode;
    if (mm != 0) return mm > 0 && mm <= 3;
    return char_on_ground(c, part);
}

int room_leader_next(wl_roomloop *rl, wl_room *room, int32_t forced, wl_pslot *cur)
{
    uint16_t start = cur != NULL ? (uint16_t)(ld16(cur, 0xa0c) + 1) : 0;
    wl_pslot *next = NULL;
    for (uint16_t i = 0; i < 8; i++) {
        wl_pslot *s = room->slot[(uint16_t)((start + i) % 8)];
        if (s->active == 1 && s->id != 100000104 && ld32(s, 0) == 0 && char_targetable(s->ch, 0, 1)) {
            next = s;
            break;
        }
    }
    if (forced != 0) {
        if (next == NULL || cur == next) return 0;
    } else {
        if (next == NULL || cur == next) goto rearm;
        if (cur != NULL && cur->ch != NULL) {
            int32_t mm = cur->ch->move_mode;
            if (mm != 0) { if (mm <= 0 || mm > 3) return 0; }
            else if (!char_on_ground(cur->ch, 0)) return 0;
        }
        if (!char_lead_ok(next->ch, 0)) return 0;
    }
    if (cur == room->leader) {
        room_leader_set(room, next);
    } else {
        void *in = ldp(cur, 0);
        pslot_input_set(cur, NULL);
        pslot_input_set(next, in);
    }
    room_formation(room);
rearm:
    if (W->room == room && room->leader != NULL && room->leader->ch != NULL) {
        const wl_char *ld = room->leader->ch;
        uint8_t *p = (uint8_t *)rl;
        st32(p, 0x118, 0); st32(p, 0x10c, 0); st16(p, 0x100, 0); st32(p, 0x104, 0); st32(p, 0x108, 0);
        st32(p, 0xf0, 0); st32(p, 0xf4, 0); st32(p, 0xf8, 0); st32(p, 0xfc, 0); st32(p, 0x110, 0); st16(p, 0x114, 0);
        wl_entity *z = NULL;
        for (uint16_t k = 0; k < 128; k++) {
            wl_entity *a = W->actors[k];
            if (a->alive != 0 && zone_hit((const wl_actor *)a, ld)) { z = W->actors[k]; break; }
        }
        memcpy(p + 0xf0, &z, 4);
        st32(p, 0xf4, 0);
    }
    wl_voice *v = vpool_take(ldp(g_eng.sys, 0x36c), 0);
    if (v != NULL) {
        g_eng.dsb_volume_5bb870(v->buf, ld32(cfg(), 0x168));
        g_eng.dsb_freq_5bb890(v->buf, 0);
        g_eng.dsb_play_5bb8c0(v->buf);
    }
    return 1;
}

int pickup_touch(const wl_entity *e, const wl_entity *o, uint16_t part)
{
    const wl_body *a = &e->body[part < e->nbody ? part : 0];
    const wl_body *b = &o->body[part < o->nbody ? part : 0];
    if (a->x > b->x + b->w) return 0;
    if (a->x + a->w <= b->x) return 0;
    if (a->y + a->margin > b->h + b->y - 1) return 0;
    return a->h + a->y - 1 > b->y + b->margin;
}

wl_entity *item_sparkle(wl_entity *e, int32_t item, int32_t sound)
{
    wl_entity *p = particle_spawn(e, e, 100137, e->body, 0, 0, e->body->facing, 1, 0, 0, 2, 0, 0, 20);
    if (p == NULL) return NULL;
    st32(p, 0x25c, item);
    if (p->code == 100044) { p->tint_f4 = ld32(g_eng.sys, 0x2758 + 4u * item); p->tint_f8 = p->tint_f4; }
    if (sound == 0) return p;
    /* (retail returns the pool's buffer table, not the particle, when every cue is busy; nothing reads it) */
    wl_voice *v = vpool_take(ldp(g_eng.sys, 0x6d0), 0);
    if (v == NULL) return p;
    void *buf = v->buf;
    int32_t vol, pan;
    if (sfx_place(p, ld32(cfg(), 0x168) - 800, 0, 0, p->body, &vol, &pan) == 0) return p;
    g_eng.dsb_volume_5bb870(buf, vol);
    g_eng.dsb_pan_5bb880(buf, pan);
    g_eng.dsb_freq_5bb890(buf, 0);
    g_eng.dsb_play_5bb8c0(buf);
    return p;
}

void room_pickups(wl_room *room)
{
    for (int i = 0; i < 8; i++) {
        const wl_pslot *s = room->slot[i];
        int lead = room->leader == s;
        if (s->active != 1) continue;
        for (int k = 0; k < 96; k++) {
            wl_entity *p = W->band96[k];
            if (p->alive == 0) continue;
            int32_t st = ld32(p, 0x204);
            if (st > 0 && st <= 4) continue;
            if (!pickup_touch(p, &s->ch->base, 0)) continue;
            int32_t amt = ld32(p, 0x200);
            if (ld32(p, 0x1fc) == 1) {
                if (amt > 0 && ld32(room, 0x4020) <= ld32(room, 0x4014)) {
                    if (lead) {
                        uint32_t now = g_eng.tick();
                        uint32_t last = (uint32_t)ld32(W, 0x102c);
                        if (last == 0 || now - last > 60000) {
                            st32(W, 0x102c, (int32_t)now);
                            world_log_line(W, S(0x8a3e08u));   /* "You have all the money you can carry." */
                        }
                    }
                    st32(p, 0x204, 4);
                } else {
                    int32_t g = ld32(room, 0x4014) + amt;
                    st32(room, 0x4014, g);
                    if (amt > 0) {
                        if (ld32(room, 0x4020) < g) g = ld32(room, 0x4020);
                        st32(room, 0x4014, g);
                        uint32_t tot = (uint32_t)(amt + ld32(room, 0x40bc));
                        st32(room, 0x40bc, tot > 0x7fffffffu ? 0x7fffffff : (int32_t)tot);
                    }
                    st32(p, 0x204, 3);
                }
            } else if (ld32(p, 0x1fc) == 2 && lead) {
                const uint8_t *tab = (const uint8_t *)g_eng.data(0x6c0cd0u);
                int32_t item = ld32(p, 0x208);
                if (ld32(tab, 0) == 0) continue;
                uint16_t r = 0;
                while (ld32(tab, 736u * r) != item)
                    if (ld32(tab, 736u * ++r) == 0) break;
                if (ld32(tab, 736u * r) == 0) continue;
                if (room_item_add(room, item, NULL, 0, 0, 0) != 0) {
                    char line[512];
                    g_eng.h5bf3ee(line, S(0x8a3e74u), (const char *)tab + 736u * r + 4);   /* "You got %s!" */
                    world_log_line(W, line);
                    item_sparkle(p, item, 0);
                    st32(p, 0x204, 3);
                } else {
                    uint32_t now = g_eng.tick();
                    uint32_t last = (uint32_t)ld32(W, 0x1028);
                    if (last == 0 || now - last > 10000) {
                        st32(W, 0x1028, (int32_t)now);
                        world_log_line(W, S(0x8a3e30u));   /* "Your bags are full, so you can not take ..." */
                    }
                }
            }
        }
    }
}

int room_item_pick(wl_room *room, wl_pslot **out, int32_t kind, int32_t a4, int32_t hp_max)
{
    int busy = W->f2770 != 0 || ld32(W, 0x2764) == 1;
    wl_pslot *best = NULL;
    int32_t top = 0;
    for (int k = 0; k < 8; k++) {
        wl_pslot *s = room->slot[k];
        if (s->active != 1 || s->id == 100000104) continue;
        int32_t v;
        if (g_eng.h47ae00(s->ch, 0, kind, (uint16_t)a4, 0, 0, 0, (intptr_t)&v, 0, 0, 1, 0, 0) == 0) continue;
        if (v <= 0 || (best != NULL && top >= v)) continue;
        top = v;
        best = s;
    }
    if (best == NULL) return -1;
    if (hp_max != 0) {
        const uint8_t *pt = (const uint8_t *)best->ch->party;
        if (hp_max < mul(ld32(pt, 0x54), 1000) / ld32(pt, 0x58)) return -1;
    }
    int32_t pick = -1, bestv = 0, bestn = 0;
    int32_t v = 0;   /* (0x47ae00 may leave it unwritten: retail's stack slot then keeps the last value) */
    for (uint16_t i = 0; i < ld16(room, 0x4010); i++) {
        const uint8_t *rec = (const uint8_t *)room + 0x2010 + 16u * i;
        if (rec[0xa] != 0) continue;
        const uint8_t *def = ldp(rec, 0xc);
        if (ld32(def, 0x5c) != 1) continue;
        int32_t cls = ld32(def, 0x2ac);
        if (cls != 0 && !(cls == 100 && ld16(room, 0x4060) == 0)) continue;
        if (ld32(def, 0x2b0) == 1 && busy) continue;
        if (ld32(def, 0x58) != kind) continue;
        g_eng.h47ae00(best->ch, 0, kind, ld32(def, 0x68), ld32(def, 0x6c), ld32(def, 0x70), ld32(def, 0x74), 0,
                      (intptr_t)&v, 0, 0, 0, 0);
        uint16_t n = ld16(rec, 8);
        if (v > bestv || (v == bestv && bestn > n) || pick < 0) {
            bestv = v;
            pick = i;
            bestn = n;
        }
    }
    if (pick < 0) return -1;
    if (out != NULL) *out = best;
    return pick;
}

/* the item-use messages (8 x 512 B) into the log; the queued use is done */
static void use_lines(wl_room *room, const char (*lines)[512], uint16_t n)
{
    for (uint16_t i = 0; i < n; i++) world_log_line(W, lines[i]);
    st16(room, 0x4070, 0);
}

void room_party_tick(wl_room *room)
{
    if (ld32(room, 0x40a4) != 0)
        for (int k = 0; k < 8; k++) {
            const wl_pslot *s = room->slot[k];
            if (s->active != 1 || s->id == 100000104) continue;
            int32_t hp = mul(ld32(s, 0xa64), 1000) / (ld32(s, 0xa68) + ld32(s, 0xa94) + ld32(s, 0xaac));
            if (hp >= 300) { st32(room, 0x4084 + 4u * k, 0); continue; }
            if (ld32(room, 0x4084 + 4u * k) != 0) continue;
            wl_voice *v = vpool_take(ldp(g_eng.sys, 0x3d8), 0);
            if (v != NULL) {
                g_eng.dsb_volume_5bb870(v->buf, ld32(cfg(), 0x168));
                g_eng.dsb_freq_5bb890(v->buf, 0);
                g_eng.dsb_play_5bb8c0(v->buf);
            }
            st32(room, 0x4084 + 4u * k, 1);
        }
    int32_t f = ld32(room, 0x40e4);
    if (f > 0) {
        f -= 16;
        if (f < 0) f = 0;
        st32(room, 0x40e4, f);
        if (f == 0) {
            st32(room, 0x40e0, 0); st32(room, 0x40e8, 0);
            st32(room, 0x40f0, 255); st32(room, 0x40f4, 255); st32(room, 0x40f8, 255); st32(room, 0x40fc, 255);
            st32(room, 0x4100, 0); st32(room, 0x40ec, 0);
        }
    }
    uint16_t mode = ld16(room, 0x4070);
    if (mode == 0) return;
    if (W->f2770 != 0 || ld32(W, 0x2764) == 1) {
        if (ld32(room, 0x4074) > 0) st32(room, 0x4074, ld32(room, 0x4074) - 1);
    } else {
        st32(room, 0x4074, 0);
    }
    if (ld32(room, 0x4074) > 0) return;
    char lines[8][512];
    uint16_t n = 0;
    if (mode == 1) {
        wl_pslot *s = NULL;
        int32_t i = room_item_pick(room, &s, 9, 0, 300);
        if (i < 0 && ld16(room, 0x405c) == 0) i = room_item_pick(room, &s, 11, 6, 0);
        if (i < 0) i = room_item_pick(room, &s, 9, 0, 750);
        if (i < 0 && ld16(room, 0x405e) == 0) i = room_item_pick(room, &s, 9, 0, 0);
        if (i >= 0 && g_eng.h5b1f10(room, (intptr_t)s, i, (intptr_t)lines, (intptr_t)&n, 0, 0) != 0) {
            use_lines(room, lines, n);
            return;
        }
        wl_voice *v = vpool_take(ldp(g_eng.sys, 0x37c), 0);
        if (v != NULL) {
            g_eng.dsb_volume_5bb870(v->buf, ld32(cfg(), 0x168));
            g_eng.dsb_freq_5bb890(v->buf, 0);
            g_eng.dsb_play_5bb8c0(v->buf);
        }
        st16(room, 0x4070, 0);
        return;
    }
    if (mode == 2) {
        const wl_pslot *s = ldp(room, 0x407c);
        if (s == NULL || (s->active == 1 && s->id != 100000104)) {
            if (g_eng.h5b1f10(room, (intptr_t)s, ld16(room, 0x4078), (intptr_t)lines, (intptr_t)&n, 0, 0) != 0) {
                use_lines(room, lines, n);
                return;
            }
        }
    }
    st16(room, 0x4070, 0);
}
