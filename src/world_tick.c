/* world_tick.c — FUN_0046cd70.  See world_tick.h. */
#include "world_tick.h"

#include <stddef.h>
#include <string.h>

#include "actor_update.h"
#include "char_ai.h"
#include "char_apply.h"
#include "engine_calls.h"
#include "entity_fx.h"
#include "entity_move.h"

#define IP(p) ((intptr_t)(p))

static int32_t rd32(const void *p, size_t off) { int32_t v; memcpy(&v, (const uint8_t *)p + off, 4); return v; }
static void    wr32(void *p, size_t off, int32_t v) { memcpy((uint8_t *)p + off, &v, 4); }
static uint16_t rd16(const void *p, size_t off) { uint16_t v; memcpy(&v, (const uint8_t *)p + off, 2); return v; }
static void    wr16(void *p, size_t off, uint16_t v) { memcpy((uint8_t *)p + off, &v, 2); }

int world_tick(void *ctl, int32_t cut)
{
    /* ── hitstop ── */
    if (rd32(ctl, 0x3c) != 0) {
        if (rd16(ctl, 0x38) == 0) { wr16(ctl, 0x38, 1); return 1; }
        uint16_t n = rd16(ctl, 0x3a);
        wr16(ctl, 0x38, 0);
        if (n != 0) { wr16(ctl, 0x3a, (uint16_t)(n - 1)); if (n - 1 == 0) wr32(ctl, 0x3c, 0); }
    }
    g_world->tick15 = (uint16_t)((g_world->tick15 + 1) & 0x7fff);

    /* ── the characters: support probe + AI ── */
    for (int i = 0; i < 32; i++) {
        wl_char *c = g_world->chars[i];
        if (c->base.alive == 0) continue;
        wl_body *b = c->base.body;
        int32_t y = cut;
        int grounded = entity_move_vertical(&c->base, b, 1, b->drop, 0, &y, NULL) == 0;
        int32_t n = c->f200;
        if (cut == 0 && c->base.coll_off != 0) n = 1;
        if (n != 0) {
            if (--n == 0) g_eng.h47b990(c, cut, IP(b), grounded);
        } else if (cut == 0) {
            char_ai(c, b, grounded);
        }
    }
    { int32_t z = 0; memcpy(g_world->grid + 0x2cb040, &z, 4); }
    if (g_world->f279c > 0) g_world->f279c = g_world->f279c - 3 < 0 ? 0 : g_world->f279c - 3;
    for (int i = 0; i < 32; i++)
        if (g_world->chars[i]->base.alive != 0) char_apply(g_world->chars[i]);

    /* ── the shots ── */
    for (int i = 0; i < 64; i++) {
        wl_entity *e = g_world->band64[i];
        if (e->alive == 0) continue;
        g_eng.h4710c0(e, IP(e->body), IP(e->body), 0, 0);
        for (uint16_t k = 1; k < e->nbody; k++)
            g_eng.h4710c0(e, IP(&e->body[k - 1]), IP(&e->body[k]), 1, k);
        if (e->snd != NULL)
            for (uint16_t m = 0; m < e->nsnd; m++) entity_snd_update(e, m);
    }
    for (int i = 0; i < 1024; i++)
        if (g_world->particles[i]->alive != 0) g_eng.h46e510(g_world->particles[i]);
    for (int i = 0; i < 96; i++)
        if (g_world->band96[i]->alive != 0) g_eng.h46da00(g_world->band96[i]);

    /* ── the actors ── */
    for (int i = 0; i < 128; i++) {
        wl_entity *a = g_world->actors[i];
        if (a->alive == 0) continue;
        wl_body *b = a->body;
        int32_t cd = rd32(a, 0x2c4);
        if (cd > 0) wr32(a, 0x2c4, cd - 1);
        if (rd32(a, 0x2e8) != 0) {
            int32_t t = rd32(a, 0x2ec);
            if (t != 0) {
                wr32(a, 0x2ec, t - 1);
            } else {
                wr32(a, 0x2ec, 16);
                int32_t box[4];
                box[0] = b->w / 2 + b->x - 32000;
                box[2] = 64000;
                box[1] = b->y + (b->h - b->margin) / 2 + b->margin - 24000;
                box[3] = 48000;
                a->faction = g_eng.h5567a0(0, IP(box), 0) != 0 ? 3 : 2;
            }
        }
        int32_t slow = b->slow, cf = b->c_flag;
        actor_update((wl_actor *)a, a->body, a->body, 0, 0);
        entity_splash(a, b, slow, cf);
        entity_fx_tick(a, b, 0);
        if (rd32(a, 0x2ac) != 0)
            for (uint16_t k = 1; k < a->nbody; k++)
                actor_update((wl_actor *)a, &a->body[k - 1], &a->body[k], 1, k);
        entity_snd_update_all(a);
    }

    /* ── the map's tile-animation phase ── */
    uint8_t *g = g_world->grid;
    uint32_t hi = 0;
    if (rd32(g, 0x28) != 0) {
        wr32(g, 0x28, 0);
        hi = (uint32_t)g_eng.h54c640(g, 0) & 0xffff0000u;
    }
    uint16_t ph = rd16(g, 0x26);
    if (ph >= 6u) {
        uint16_t big = (uint16_t)(rd16(g, 0x24) + 1);
        wr16(g, 0x24, big);
        wr16(g, 0x26, 0);
        if (big >= 6u) wr16(g, 0x24, 0);
        return 0;
    }
    wr16(g, 0x26, (uint16_t)(ph + 1));
    return (int)(hi | (uint32_t)(ph + 1));
}
