/* behave_ops.c — the behaviour AI's helpers.  See behave_ops.h.
 *
 * Transcribed from the decompile, checked against the asm (0x47e330 keeps its candidates as
 * (char, score) pairs on a 0x134-byte frame; its u16 counters compare unsigned; the part
 * index it hands the box helpers is the caller's full dword, which they truncate and clamp
 * against their own body counts). */
#include "behave_ops.h"

#include <stddef.h>
#include <string.h>

#include "char_ai.h"
#include "behave_helpers.h"
#include "engine_calls.h"
#include "map_grid.h"
#include "npc_ai.h"
#include "rng.h"

#define IP(p) ((intptr_t)(p))

static int32_t rnd(void) { return (int32_t)rng_rand(); }
static int32_t mul(int32_t a, int32_t b) { return (int32_t)((uint32_t)a * (uint32_t)b); }
static int32_t iabs(int32_t v) { return v < 0 ? (int32_t)(0u - (uint32_t)v) : v; }
static int32_t gi32(size_t off) { int32_t v; memcpy(&v, g_world->grid + off, 4); return v; }
static uint16_t gu16(size_t off) { uint16_t v; memcpy(&v, g_world->grid + off, 2); return v; }
static int32_t clampi(int32_t v, int32_t hi) { if (v < 0) v = 0; return hi >= v ? v : hi; }
static int32_t region_d(int32_t col, int32_t row) { return gu16(MG_REGION_D + 2u * (uint32_t)(row + (col << 7))); }

int faction_rel(const wl_char *self, int32_t a, int32_t b)
{
    int r;
    if (a != 0 && b != 0) r = a == 3 || b == 3 ? 1 : (a == b) + 1;
    else r = 0;
    if (self->base.coll_off != 0 && (r == 1 || r == 2)) return 1;
    return r;
}

wl_char *char_engaged_target(const wl_char *c)
{
    if (c->f200 == 0 || c->f14288 == 1) return c->target;
    return NULL;
}

/* the status table 0x628790: 18 records of 0x8c bytes, the first dword a weight */
int32_t char_status_threat(const wl_char *c)
{
    const uint8_t *t = g_eng.data(0x628790u);
    int32_t m = 0;
    for (int k = 0; k < 18; k++) {
        int32_t w; memcpy(&w, t + 0x8c * k, 4);
        if (wl_party_status(c->party, k) != 0 && m <= w) m = w;
    }
    return m;
}

intptr_t behave_set_target(wl_char *self, wl_char *t)
{
    self->target = t;
    if (t == NULL) return 0;
    self->target_x = t->base.body->x;
    self->target_y = t->base.body->y + t->base.body->margin;
    return IP(t->base.body);
}

int behave_c_check(const wl_char *self)
{
    return self->nav_c_check != 0 && self->fc7ec == 0 && self->party->f45e == 0;
}

int behave_set_state(wl_char *self, int16_t s)
{
    self->ai_state = (uint16_t)s;
    self->ai_sub = 0;
    self->ai_t = 0;
    return 0;
}

int behave_step_ok(wl_char *self, const wl_body *b, int32_t dist, int32_t dir, int32_t strict, int32_t deep,
                   int32_t need_floor)
{
    int32_t mm = self->move_mode;
    if (mm > 0 && mm <= 2) need_floor = 0;
    else if (mm == 3) {
        if (need_floor == 0) return 1;
        need_floor = 0;
    } else need_floor = 1;

    int32_t x = dir == 1 ? b->x + b->w + dist - 1 : b->x - dist;
    int32_t col = x / 3200, span = deep != 0 ? 5 : 2, rows = gi32(MG_DIM1), h = b->h;
    int32_t top = clampi(b->y / 3200, rows - 1), bot = clampi((h + b->y - 1) / 3200, rows - 1);
    if (strict != 0) {
        if (x < 0 || col >= gi32(MG_DIM0)) return 0;
    } else {
        col = clampi(col, gi32(MG_DIM0) - 1);
    }
    if (region_d(col, top) == 1) return 0;
    if (region_d(col, bot) == 1) bot--;
    if (need_floor == 0) return 1;

    const wl_entity *at = b->attach;
    if (at != NULL) {
        switch ((uint32_t)at->ride_mode) {
        case 3: return 1;
        case 0: case 1:
            if (self->platform != at) return 1;
            /* fall through */
        default:
            return !(x > at->body->x + at->body->w || x < at->body->x);
        }
    }
    for (int32_t r = bot + 1; r <= bot + span; r++) {
        if (r >= rows) return 0;
        if (region_d(col, r) != 0) {
            if (self->nav_c_check == 0 || self->fc7ec != 0 || self->party->f45e != 0) return 1;
            int32_t c;
            if (grid_c_query(g_world->grid, 3200 * col, 3200 * r - h, &c) == 0) return 1;
            return c == 0 || c == 2;
        }
    }
    return 0;
}

wl_char *behave_pick_char(wl_char *self, int32_t fac, int32_t rel, uint32_t part, int32_t reach, int32_t crowd,
                          int32_t near_leader, int32_t threat)
{
    struct { wl_char *c; int32_t score; } cand[32];
    wl_char *lead = self->leader, *pick = NULL;
    int32_t best = 0, least_i = -1, rx, ry;
    uint16_t n = 0, least = 0, ties = 1;

    if (lead != NULL && near_leader != 0) {
        uint32_t p = part;
        uint16_t i = (uint16_t)p;
        if (i >= lead->base.nbody) {
            i = 0;
            g_eng.h407f30(g_eng.log, IP("GetHeadLeftRight State Count Over."), 0, lead->base.code);
        }
        if (lead->base.body[i].facing != lead->fc77c) p = 0;
        int32_t off = self->side == 3 ? self->side_dist : -self->side_dist;
        off -= part_width(&self->base, (uint16_t)p) / 2;
        rx = part_center_x(&lead->base, (uint16_t)p) + off;
        ry = part_bottom(&lead->base, (uint16_t)p);
    } else {
        const wl_body *sb = self->base.body;
        rx = sb->x + sb->w / 2;
        ry = sb->y + (sb->h - sb->margin) / 2 + sb->margin;
    }

    for (int i = 0; i < 32; i++) {
        wl_char *c = g_world->chars[i];
        if (c->base.alive != 1) continue;
        const wl_body *cb = c->base.body;
        if (cb->active != 1 || cb->f7e != 0 || c->party->busy != 0 || c == self) continue;
        if (faction_rel(self, fac, c->base.faction) != rel) continue;
        int32_t dx = iabs(reach - iabs(cb->x + cb->w / 2 - rx));
        int32_t dy = iabs(cb->y + (cb->h - cb->margin) / 2 - ry + cb->margin);
        if (self->f14288 == 1) {
            if (dx >= 96000 || dy >= 64000) continue;
        } else if (dx >= 48000 || dy >= 32000) {
            continue;
        }
        cand[n].score = dx + mul(rnd(), 6400) / 0x8000 + 2 * dy;
        cand[n].c = c;
        if (self->base.coll_off != 0) {
            int32_t a = self->base.faction, o = c->base.faction;
            if (a != 0 && o != 0 && (a == 3 || o == 3 || a != o)) cand[n].score += 32000;
        } else if (threat != 0) {
            cand[n].score += mul(char_status_threat(c), 48000) / 1000;
        }
        if (crowd == 1) {
            uint16_t k = 0;
            for (int j = 0; j < 32; j++) {
                wl_char *o = g_world->chars[j];
                if (o->base.alive == 1 && o->base.body->active == 1 && o->party->busy == 0 && o != self &&
                    faction_rel(self, self->base.faction, o->base.faction) == 2 && char_engaged_target(o) == c)
                    k++;
            }
            if (least > k || least_i == -1) {
                least = k;
                least_i = n;
            } else if (least == k) {
                ties++;
            }
        }
        n++;
    }
    if (least_i != -1 && ties < n) cand[least_i].score -= 16000;
    for (uint16_t k = 0; k < n; k++)
        if (best > cand[k].score || pick == NULL) { best = cand[k].score; pick = cand[k].c; }
    return pick;
}

int behave_walk_to(wl_char *self, wl_body *b, int32_t gr)
{
    if (self->ai_state == 1) {
        beh_441f50((wl_char *)(self), (int32_t *)(IP(b)), gr);
        return 1;
    }
    if (self->ai_state != 7) return 0;
    int32_t tx = self->walk_x;
    cmd_clear(&self->cmd);
    if (b->attach == NULL) goto stop;
    if (b->x >= tx) {
        if (behave_step_ok(self, b, 1600, 3, 0, 1, 0) == 0) {
            behave_set_state(self, 0);
            return 1;
        }
    } else if (self->move_mode != 3) {
        int32_t chk = self->move_mode > 0 && self->move_mode <= 2 ? 0 : 1;
        if (npc_floor_ahead(self, b, b->w + b->x + 1599, 0, chk, 5) == 0) goto stop;
    }
    npc_navigate(self, tx, 1600, b, gr, 0, 0, 0, 0, 0);
    if (iabs(tx - b->x) < 1600) behave_set_state(self, 0);
    return 1;
stop:
    self->ai_state = 0;
    self->ai_sub = 0;
    self->ai_t = 0;
    return 1;
}

int behave_pace(wl_char *self, wl_body *b, int32_t gr, int16_t hold, int32_t chance)
{
    int coin = 0, turned = 0;
    if (self->side_t != 0) self->side_t--;
    if (chance != 0) coin = chance > mul(rnd(), 1000) / 0x8000;
    int right = self->side == 1;
    int32_t post = right ? self->f14264 : self->f14268;
    if (self->side_t == 0) {
        int turn = 1;
        if (iabs(b->x - post) > 3200) {
            int floor = 1;
            if (self->move_mode != 3) {
                int32_t chk = self->move_mode > 0 && self->move_mode <= 2 ? 0 : 1;
                floor = npc_floor_ahead(self, b, right ? b->w + b->x + 1599 : b->x - 1600, 1, chk, 2) != 0;
            }
            if (floor) turn = coin;
        }
        if (turn) {
            self->side = right ? 3 : 1;
            self->side_t = (uint16_t)hold;
            turned = 1;
        }
    }
    npc_navigate(self, post, 3200, b, gr, 0, 0, 0, 1, 0);
    return turned;
}

intptr_t behave_go_home(wl_char *self, wl_body *b, int32_t gr)
{
    wl_entity *at = b->attach;
    if (at != NULL) {
        switch ((uint32_t)at->ride_mode) {
        case 3: break;
        case 0: case 1:
            if (self->platform != at) break;
            return IP(at);
        default:
            return IP(at);
        }
    }
    return npc_navigate(self, self->f14268, 800, b, gr, 0, 1, self->f1426c, 0, 0);
}

int behave_hold(wl_char *self, wl_body *b, int32_t gr, int32_t dec)
{
    int32_t t = self->f14274;
    if (t <= 0) {
        self->f14270 = 1;
        return 0;
    }
    t -= dec;
    self->f14274 = t < 0 ? 0 : t;
    npc_navigate(self, b->x, 800, b, gr, 0, 0, 0, 0, 0);
    return 1;
}

void behave_follow(wl_char *self, wl_body *b, int32_t gr, uint16_t part)
{
    wl_char *lead = self->leader;
    if (lead == NULL) return;
    uint16_t i = part < lead->base.nbody ? part : 0;
    int32_t lx = lead->base.body[i].x, d = lx - b->x;
    if (iabs(d) >= self->follow_dist) {
        npc_navigate(self, lx, 1600, b, gr, 0, 0, 0, 0, 0);
        return;
    }
    int32_t face = d <= 0 ? 3 : 1;
    if (b->facing != face) self->cmd.lr = face == 1 ? 2 : 1;
}
