/* char_state.c — per-character state helpers.  See char_state.h.
 *
 * Transcribed from the asm (0x4874b0 stores the action length as a u16, which the
 * decompile shows as a dword; 0x41e180 clears u16 fields; 0x487530 writes the u16 at
 * +0xc7e8 only). */
#include "char_state.h"

#include <string.h>

#include "char_integrate.h"
#include "engine_calls.h"
#include "entity_move.h"
#include "map_grid.h"
#include "npc_ai.h"
#include "rng.h"

static int32_t mul(int32_t a, int32_t b) { return (int32_t)((uint32_t)a * (uint32_t)b); }
static int32_t rnd(void) { return (int32_t)rng_rand(); }
static int32_t gi32(size_t off) { int32_t v; memcpy(&v, g_world->grid + off, 4); return v; }
static uint16_t gu16(size_t off) { uint16_t v; memcpy(&v, g_world->grid + off, 2); return v; }
static int32_t clampi(int32_t v, int32_t hi) { if (v < 0) v = 0; return hi >= v ? v : hi; }
static int32_t region_d(int32_t col, int32_t row) { return gu16(MG_REGION_D + 2u * (uint32_t)(row + (col << 7))); }
/* region B's +0xc dword (the cell's object) */
static int32_t region_b_obj(int32_t col, int32_t row) { return gi32(MG_REGION_B + 0xc + 16u * (uint32_t)(row + (col << 7))); }

int char_add_hp(wl_char *c, int32_t d, int32_t *out, int32_t dry)
{
    wl_party *p = c->party;
    if (p->busy != 0) {
        *out = 0;
        return 0;
    }
    int32_t hp = p->hp, v = d;
    if (d < 0) {
        if (hp + d < 0) v = -hp;
    } else {
        int32_t max = p->hp_max + p->hp_bonus1 + p->hp_bonus2;
        if (max < d + hp) v = p->hp_max - hp + p->hp_bonus1 + p->hp_bonus2;
    }
    if (dry == 0) p->hp = hp + v;
    if (out != NULL) *out = v;
    return c->party->hp <= 0;
}

int32_t char_add_mp(wl_char *c, int32_t d, int32_t *out, int32_t dry)
{
    wl_party *p = c->party;
    if (p->busy != 0) return d;
    int32_t mp = p->mp, v = d;
    if (d < 0) {
        if (mp + d < 0) v = -mp;
    } else {
        int32_t max = p->mp_max + p->mp_bonus1 + p->mp_bonus2;
        if (max < d + mp) v = p->mp_max - mp + p->mp_bonus1 + p->mp_bonus2;
    }
    if (dry == 0) p->mp = mp + v;
    if (out != NULL) *out = v;
    return v;
}

int char_status_stuck(const wl_char *c)
{
    const uint8_t *t = g_eng.data(0x628764u);
    for (int k = 0; k < 18; k++) {
        int32_t f; memcpy(&f, t + 0x8c * k, 4);
        if (wl_party_status(c->party, k) != 0 && f != 0) return 1;
    }
    return 0;
}

int char_targetable(const wl_char *c, int32_t any_busy, int32_t any_7e)
{
    if (c->base.alive != 1 || c->base.body->active != 1) return 0;
    if (any_7e == 0 && c->base.body->f7e != 0) return 0;
    if (any_busy == 0 && c->party->busy != 0) return 0;
    return 1;
}

int32_t char_mods_reset(wl_char *c)
{
    c->fc7d8 = 0;
    c->fc7dc = 0;
    c->fc7e4 = 0;
    c->fc7e0 = 0;
    c->max_air_jumps = 0;
    for (int i = 0; i < 12; i++) c->mods[i] = 1000;
    return 1000;
}

void char_ai_reset(wl_char *c, int32_t full)
{
    c->f1429c = 0;
    c->f147a0 = 0;
    c->ai_state = 0;
    c->ai_sub = 0;
    c->ai_t = 0;
    c->target_t = 0;
    c->f147ac = 0;
    c->f147f4 = 0;
    c->think_t = 0;
    c->f14802 = 0;
    c->f14270 = 0;
    c->target = NULL;
    c->f14234 = 0;
    c->f14236 = 0;
    c->f14804 = 0;
    c->f14288 = 0;
    c->pal = NULL;
    c->side_t = 0;
    c->calm_t = 0;
    c->f14294 = 0;
    c->f1423c = 0;
    c->boost_speed = 0;
    c->boost_accel = 0;
    if (full == 0) return;
    c->f14830 = 0;
    c->f1482e = 0;
    c->f14274 = 0;
    c->engage_t = 0;
    for (uint16_t i = 0; i < c->ncd; i++) {
        uint8_t *r = (uint8_t *)&c->cd0 + 0x140u * i;
        memcpy(r, r + 4, 2);
    }
}

void char_respawn_mark(wl_char *c, wl_body *b)
{
    int32_t y;
    if (entity_move_vertical(&c->base, b, 1, b->drop, 0, &y, NULL) != 0) return;
    int32_t row = (b->y + b->h + 1) / 3200;
    if (row < 0 || row >= gi32(MG_DIM1)) return;
    int32_t cols = gi32(MG_DIM0), above = clampi(row - 1, gi32(MG_DIM1) - 1);
    int32_t c0 = clampi(b->x / 3200, cols - 1);
    if (region_d(c0, row) == 0 || region_b_obj(clampi(c0, cols - 1), above) != 0) return;
    int32_t c1 = clampi((b->x + b->w) / 3200, cols - 1);
    if (region_d(c1, row) == 0 || region_b_obj(clampi(c1, cols - 1), above) != 0) return;
    c->respawn_x = b->x;
    c->respawn_y = b->y + b->h - 1;
}

int char_pit_check(wl_char *c, const wl_body *b, int32_t *kind)
{
    if (c->nav_c_check == 0 || c->fc7ec != 0 || c->party->f45e != 0 || (uint32_t)b->slow < 2u) {
        c->pit_t = 0;
        return 0;
    }
    int32_t v;
    grid_c_query(g_world->grid, b->x + b->w / 2, b->h + b->y - 1, &v);
    switch ((uint32_t)v) {
    case 0: case 2:
        c->pit_t = 0;
        return 0;
    case 1: case 3:
        if (++c->pit_t <= 20) return 0;
        c->pit_t = 0;
        *kind = v;
        return 1;
    default:
        return 0;
    }
}

int32_t char_set_look_up(wl_char *c, int16_t pose, const void *clip0, const void *clip1, int16_t len,
                         int32_t chance, int32_t a7)
{
    c->clip_up0 = clip0;
    c->clip_up1 = clip1 != NULL ? clip1 : clip0;
    c->up_pose = (uint16_t)pose;
    c->up_len = (uint16_t)len;
    c->up_assist = 1;
    c->nav_jump_chance = chance;
    c->f15a24 = a7;
    return chance;
}

int char_force_action(wl_char *c, int16_t pose, int32_t len)
{
    body_action_start(c->base.body, pose, len);
    return 0;
}

int body_slowed(const wl_body *b) { return (uint32_t)b->slow > 1u; }

int32_t grid_b_word(const uint8_t *grid, int32_t x, int32_t y)
{
    int32_t v;
    memcpy(&v, grid + MG_DIM0, 4);
    int32_t col = clampi(x / 3200, v - 1), row = y / 3200;
    memcpy(&v, grid + MG_DIM1, 4);
    if (row < 0 || row >= v) return 0;
    memcpy(&v, grid + MG_REGION_B + 4 + 16u * (uint32_t)(row + (col << 7)), 4);
    return v;
}

int32_t grid_c_at_feet(int32_t a1, const wl_body *b)
{
    (void)a1;   /* both of retail's arms compute the same cell */
    int32_t v;
    grid_c_query(g_world->grid, b->x + b->w / 2, b->h + b->y - 1, &v);
    return v;
}

int32_t entity_scatter(wl_entity *e, int32_t vx0, int32_t vxr, int32_t vy0, int32_t vyr)
{
    wl_body *b = e->body;
    b->vvel = mul(rnd(), vyr) / 0x8000 + vy0;
    b->vel_x = mul(rnd(), vxr) / 0x8000 + vx0;
    return b->vel_x;
}

int32_t sys_by_mode(const void *sys, int32_t a, int32_t b, int32_t c)
{
    int32_t m; memcpy(&m, (const uint8_t *)sys + 0x289c, 4);
    switch (m) {
    case 2: case 3: return a;
    case 4: return b;
    case 5: return c;
    default: return 0;
    }
}

void entity_anchor(const wl_entity *e, int32_t x, int32_t y, int32_t anchor, int32_t *ox, int32_t *oy)
{
    switch (anchor) {
    case 0: *ox = x; *oy = y; break;
    case 1: *ox = x - 100 * e->dst_x; *oy = y - 100 * e->dst_y; break;
    case 2: *ox = x - e->width / 2; *oy = y - e->height / 2; break;
    case 3: *ox = x; *oy = y - e->foot - e->height; break;
    case 4: *ox = x - e->width / 2; *oy = y; break;
    case 5: *ox = x - e->width / 2; *oy = y - e->foot - e->height; break;
    default: break;
    }
}

int32_t entity_set_track(wl_entity *e, uint16_t i, int16_t a, int16_t b, uint8_t on, int32_t f0c, int32_t f08, int32_t f10)
{
    e->trk[i].a = (uint16_t)a;
    e->trk[i].b = (uint16_t)b;
    e->trk[i].on = on;
    e->trk[i].f0c = f0c;
    e->trk[i].f08 = f08;
    e->trk[i].f10 = f10;
    return f10;
}

int char_skill_avail(wl_char *c, const wl_skill *k, const wl_body *b, int32_t grounded, int32_t probing)
{
    if (k == NULL) return 0;
    switch (k->cls) {
    case 0xf00000:
        break;
    case 0xf00001:
        if (c->party->f466 != 0 || party_find(c, 50013, 0, NULL, 0) == NULL) return 0;
        break;
    case 0xf00003:
        if (c->party->member != 1) return 0;
        /* fall through */
    case 0xf00002:
        if (c->party->f466 == 0) return 0;
        break;
    default:
        if (k->cls != c->party->member) return 0;
        break;
    }
    switch (k->mode) {
    case 1: case 2: break;
    case 3: if (probing != 0) return 0; break;
    case 0xff: if (c->base.coll_off == 0) return 0; break;
    default: return 0;
    }
    int32_t req = k->pose_req;
    if (req == 0) {
        if (b->pose != 0 && b->act_len > 200) return 0;
    } else {
        if (b->act_64 < k->pose_lo || b->act_64 > k->pose_hi) return 0;
        if (b->pose != (uint32_t)req) {
            if (req != 10101) return 0;
            if (b->pose == 10115) {
                if (b->act_arg != 0) return 0;
            } else if (b->pose != 10122) {
                return 0;
            }
        }
    }
    switch (k->ground) {
    case 1: if (grounded == 0) return 0; break;
    case 2: if (grounded != 0) return 0; break;
    case 3:
        if (grounded == 0) return 0;
        if (b->vel_x < mul(c->max_speed, 900) / 1000) return 0;
        break;
    default: break;
    }
    return 1;
}

wl_char *party_find(const wl_char *c, int32_t code, int16_t pose, uint16_t *f58, uint16_t status)
{
    if (c->fc794 == 0) return NULL;
    for (uint16_t i = 0; i < 8; i++) {
        const uint8_t *reg; memcpy(&reg, (const uint8_t *)(intptr_t)c->fc794 + 0x9ec, sizeof reg);
        const uint8_t *ent; memcpy(&ent, reg + 0x4030 + 4u * i, sizeof ent);
        int32_t on; memcpy(&on, ent + 0x9c4, 4);
        if (on != 1) continue;
        wl_char *o; memcpy(&o, ent + 0x9f4, sizeof o);
        if (o == NULL) continue;
        if (code != 0 && o->base.code == code) return o;
        if (pose != 0) {
            if (f58 != NULL) *f58 = o->base.body->f58;
            if (o->base.body->pose == (uint16_t)pose) return o;
        }
        if (status != 0 && wl_party_status(o->party, status) != 0) return o;
    }
    return NULL;
}

int32_t grid_floor_below(const wl_body *b, int32_t *row_out)
{
    int32_t cols = gi32(MG_DIM0), x = b->x;
    int32_t c0 = clampi(x / 3200, cols - 1), c1 = clampi((b->w + x - 1) / 3200, cols - 1);
    int32_t row = (b->h + b->y - 1) / 3200 + 1;
    while (row < gi32(MG_DIM1) && region_d(c0, row) == 0 && region_d(c1, row) == 0) row++;
    if (row_out != NULL) *row_out = row;
    return 3200 * row - b->h - b->y;
}
