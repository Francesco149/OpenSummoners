/* room_loop.c — the room loop's per-tick calls.  See room_loop.h.
 *
 * Transcribed from the asm.  GetTickCount is read exactly where retail reads it (rhost's
 * virtual clock counts calls). */
#include "room_loop.h"

#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "actor_update.h"
#include "char_ai.h"
#include "char_behave.h"
#include "char_physics.h"
#include "engine_calls.h"
#include "entity_fx.h"
#include "entity_move.h"
#include "gimmick.h"
#include "grid_refresh.h"
#include "input.h"

#define IP(p) ((intptr_t)(p))

static int32_t iabs(int32_t v) { return v < 0 ? (int32_t)(0u - (uint32_t)v) : v; }
static void *sysp(uint32_t off) { void *p; memcpy(&p, (const uint8_t *)g_eng.sys + off, sizeof p); return p; }

void room_input_update(wl_room *r)
{
    for (uint16_t j = 0; j < r->nin; j++) {
        input_mgr *m = &r->in[j];
        int32_t old[11];
        for (int k = 0; k < 11; k++) {
            old[k] = m->axis_held[k];
            m->axis_held[k] = 0;
        }
        if (m->src_kb != 0) g_eng.h46a880(m, IP(sysp(0x2878)));
        for (uint32_t k = 0; k < 2; k++) {
            void *joy = sysp(0x287c + 4 * k);
            if (m->src_joy[k] != 0 && joy != NULL) g_eng.h468b10(m, IP(joy), 0);
        }
        uint32_t now = g_eng.tick();
        for (int k = 0; k < 11; k++) {
            if (m->axis_held[k] == 0) m->axis_held_b[k] = 0;
            else if (old[k] == 0) m->axis_held_b[k] = (int32_t)now;
        }
    }
}

int room_char_step(wl_char *c, int32_t cut)
{
    wl_body *b = c->base.body;
    int32_t y;
    int gr = entity_move_vertical(&c->base, b, 1, b->drop, 0, &y, NULL) == 0;
    int32_t mode = c->f200;
    if (cut == 0 && c->base.coll_off != 0) mode = 1;
    if (mode == 0) {
        if (cut == 0) return char_ai(c, b, gr);
    } else if (mode == 1) return char_behave(c, cut, b, gr);
    return gr;
}

void room_actor_step(wl_actor *a)
{
    wl_body *b = a->base.body;
    if (a->cooldown > 0) a->cooldown--;
    if (a->f2e8 != 0) {
        if (a->f2ec != 0) a->f2ec--;
        else {
            a->f2ec = 16;
            int32_t box[4];
            box[0] = b->w / 2 + b->x - 32000;
            box[2] = 64000;
            box[3] = 48000;
            box[1] = b->y + (b->h - b->margin) / 2 + b->margin - 24000;
            a->base.faction = char_in_rect(0, box, 0) != 0 ? 3 : 2;
        }
    }
    int32_t slow = b->slow, cf = b->c_flag;
    actor_update(a, a->base.body, a->base.body, 0, 0);
    entity_splash(&a->base, b, slow, cf);   /* inlined in retail (the same records) */
    entity_fx_tick(&a->base, b, 0);
    if (a->multipart != 0)
        for (uint16_t k = 1; k < a->base.nbody; k++) actor_update(a, &a->base.body[k - 1], &a->base.body[k], 1, k);
    if (a->base.snd != NULL)
        for (uint16_t i = 0; i < a->base.nsnd; i++) entity_snd_update(&a->base, i);
}

void room_grid_anim(uint8_t *grid)
{
    int32_t f; memcpy(&f, grid + 0x28, 4);
    if (f != 0) {
        f = 0;
        memcpy(grid + 0x28, &f, 4);
        grid_refresh(grid, 0);
    }
    uint16_t ph, big; memcpy(&ph, grid + 0x26, 2);
    if (ph < 6) {
        ph++;
        memcpy(grid + 0x26, &ph, 2);
        return;
    }
    memcpy(&big, grid + 0x24, 2);
    big++;
    ph = 0;
    memcpy(grid + 0x24, &big, 2);
    memcpy(grid + 0x26, &ph, 2);
    if (big >= 6) { big = 0; memcpy(grid + 0x24, &big, 2); }
}

int room_party_down(const wl_room *r)
{
    int up = 0;
    for (int k = 0; k < 8; k++) {
        const wl_pslot *s = r->slot[k];
        if (s->active != 1 || s->id == 100000104) continue;
        const wl_char *c = s->ch;
        if (c->base.alive == 1 && c->base.body->active == 1 && c->party->busy == 0) up = 1;
    }
    return up ? 0 : 4;
}

void room_lead(wl_roomloop *rl, const wl_char *c, const wl_char *partner, int32_t mode)
{
    int32_t t = -30000;
    if (mode == 1) {
        if (c != NULL && partner != NULL) {
            const wl_body *b = c->base.body, *p = partner->base.body;
            t = p->w / 2 - b->w / 2 - b->x + p->x - 30000;
            if (t < -52000) t = -52000;
            else if (t > -8000) t = -8000;
        }
    } else if (mode == 2) {
        if (c != NULL) {
            if (c->base.nbody == 0) {
                char buf[256];
                snprintf(buf, sizeof buf, "%d", c->base.code);
                g_eng.h408b90(g_eng.log, IP(g_eng.data(0x85451cu)), 0, IP(buf));   /* "GetHeadLeftRight State Count Over." */
            }
            t = c->base.body->facing == 1 ? -8000 : -52000;
        }
    } else return;
    int32_t cur = rl->lead, step = iabs(cur - t) / 40;
    if (cur < t) rl->lead = cur + step <= t ? cur + step : t;
    else rl->lead = cur - step >= t ? cur - step : t;
}

/* a timed-sound record (sys+0xaa8 -> +0x8, 24 B) */
typedef struct tsound {
    void    *buf;     /* +0x00 */
    int32_t  vol;     /* +0x04 */
    uint32_t dur;     /* +0x08  ms it plays */
    uint32_t t0;      /* +0x0c */
    int32_t  state;   /* +0x10  1 start, 2 playing, 3 fading */
    int32_t  fade;    /* +0x14  volume lost per tick */
} tsound;

void timed_sounds_tick(void *sys)
{
    const uint8_t *s = sys;
    void *bgm; memcpy(&bgm, s + 0x2884, sizeof bgm);
    if (bgm != NULL) g_eng.h5bc0c0(bgm);
    uint8_t *l; memcpy(&l, s + 0xaa8, sizeof l);
    if (l == NULL) return;
    tsound *rec; memcpy(&rec, l + 8, sizeof rec);
    if (rec == NULL) return;
    uint32_t now = g_eng.tick();
    uint16_t n; memcpy(&n, l + 4, 2);
    for (uint16_t i = 0; i < n; i++, memcpy(&n, l + 4, 2)) {
        memcpy(&rec, l + 8, sizeof rec);
        tsound *t = &rec[i];
        if (t->buf == NULL) continue;
        switch (t->state) {
        case 1:
            t->t0 = g_eng.tick();
            t->state = 2;
            g_eng.dsb_volume_5bb870(t->buf, t->vol);
            g_eng.dsb_play_5bb8c0(t->buf);
            break;
        case 2:
            if (now - t->t0 > t->dur) {
                t->state = 3;
                t->fade = 200;
            }
            break;
        case 3: {
            int32_t v = t->vol - t->fade;
            if (v < -10000) v = -10000;
            t->vol = v;
            g_eng.dsb_volume_5bb870(t->buf, v);
            if (t->vol > -10000) break;
            void *b = t->buf;
            if (b != NULL) {
                g_eng.dsb_release_5bb3a0(b);
                g_eng.op_delete(b);
            }
            memset(t, 0, sizeof *t);
            break;
        }
        default: break;
        }
    }
}
