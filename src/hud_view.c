/* hud_view.c — the HUD's per-frame animation.  See hud_view.h.  Transcribed from the asm. */
#include "hud_view.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "actor_update.h"     /* world_scene_running 0x47d740 */
#include "camera_follow.h"    /* view_box_visible 0x4766a0 */
#include "char_ai.h"          /* party_slot_get 0x479d30 */
#include "engine_calls.h"
#include "npc_ai.h"           /* part_center_y 0x44e680 */
#include "world_layout.h"

static int32_t  ld32(const void *p, uint32_t off) { int32_t v; memcpy(&v, (const uint8_t *)p + off, 4); return v; }
static uint16_t ld16(const void *p, uint32_t off) { uint16_t v; memcpy(&v, (const uint8_t *)p + off, 2); return v; }
static void st32(void *p, uint32_t off, int32_t v) { memcpy((uint8_t *)p + off, &v, 4); }
static void st16(void *p, uint32_t off, uint16_t v) { memcpy((uint8_t *)p + off, &v, 2); }
static void *ldp(const void *p, uint32_t off) { void *v; memcpy(&v, (const uint8_t *)p + off, sizeof v); return v; }
static void stp(void *p, uint32_t off, const void *v) { memcpy((uint8_t *)p + off, &v, sizeof v); }
static const uint8_t *D(uint32_t va) { return g_eng.data(va); }
static int32_t dec_to0(int32_t v, int32_t by) { v -= by; return v < 0 ? 0 : v; }
static int32_t inc_to1000(int32_t v, int32_t by) { v += by; return v > 1000 ? 1000 : v; }

/* the HP per mille (HP +0x54 over +0x84 + +0x9c + +0x58, at least 1) */
static int32_t hp_ratio(const uint8_t *st)
{
    int32_t m = ld32(st, 0x9c) + ld32(st, 0x84) + ld32(st, 0x58);
    if (m < 1) m = 1;
    return 1000 * ld32(st, 0x54) / m;
}

/* move c toward t by a tenth of the gap (at least 5) without passing it */
static int32_t ease(int32_t c, int32_t t)
{
    int32_t d = abs(c - t) / 10;
    if (d < 5) d = 5;
    if (c < t) {
        c += d;
        if (c > t) c = t;
    } else if (c > t) {
        c -= d;
        if (t > c) c = t;
    }
    return c;
}

int32_t hud_ease(int32_t *v, int32_t t, int32_t max)
{
    int32_t c = ease(*v, t);
    *v = max >= c ? c : max;
    return max;
}

int32_t stats_mp_ratio(const uint8_t *st)
{
    int32_t m = ld32(st, 0x60) + ld32(st, 0x88) + ld32(st, 0xa0);
    if (m < 1) m = 1;
    return 1000 * ld32(st, 0x5c) / m;
}

int32_t hud_status_icon(uint8_t *p, uint16_t k)
{
    const uint8_t *st = ldp(ldp(p, 4), 0x750);
    uint16_t val = ld16(st, 0x448u + 2u * k), mx = ld16(st, 0x46cu + 2u * k);
    uint16_t big = val <= mx ? mx : val, cap = big != 0 ? (val > mx ? val : mx) : 1;
    int32_t r = 35 * k;
    if (ld16(D(0x62875cu), 140u * k) == 0) return r;
    int32_t n = ld32(p, 0x18c);
    for (int32_t i = 0; i < n; i++) {
        uint8_t *e = p + 20 * i;
        if (ld16(e, 0x24) != k || ld32(e, 0x30) != 0) continue;
        st16(e, 0x26, val);
        st16(e, 0x28, cap);
        if (val == 0) st32(e, 0x30, 1);
        return (int32_t)(intptr_t)e;
    }
    if (val != 0 && n < 18) {
        st32(p, 0x18c, n + 1);
        uint8_t *e = p + 20 * n;
        st16(e, 0x24, k);
        st16(e, 0x28, cap);
        st16(e, 0x26, val);
        st32(e, 0x2c, 0);
        st32(e, 0x34, 0);
        st32(e, 0x30, 0);
        return 5 * n + 10;
    }
    return n;
}

int32_t hud_status_drop(uint8_t *p, uint16_t i)
{
    for (; (int32_t)i < ld32(p, 0x18c) - 1; i++) {
        uint8_t *e = p + 20u * i;
        st16(e, 0x24, ld16(e, 0x38));
        st16(e, 0x28, ld16(e, 0x3c));
        st16(e, 0x26, ld16(e, 0x3a));
        st32(e, 0x2c, ld32(e, 0x40));
        st32(e, 0x34, ld32(e, 0x48) + 1000);
        st32(e, 0x30, ld32(e, 0x44));
    }
    st32(p, 0x18c, ld32(p, 0x18c) - 1);
    return ld32(p, 0x18c);
}

/* a panel's frame: its bars, its quick slot / class change clock, its mood (cross-faded), its status icons */
static void panel_tick(uint8_t *p)
{
    const uint8_t *st = ldp(ldp(p, 4), 0x750);
    int32_t v = ld32(p, 8);
    hud_ease(&v, hp_ratio(st), 1000);
    st32(p, 8, v);
    v = ld32(p, 0xc);
    hud_ease(&v, stats_mp_ratio(st), 1000);
    st32(p, 0xc, v);
    if (ld16(p, 0x10) != party_slot_get((wl_party *)st) || (int32_t)ld16(p, 0x12) != ld32(st, 0x140)) {
        st16(p, 0x10, party_slot_get((wl_party *)st));
        st16(p, 0x12, ld16(st, 0x140));
        st16(p, 0x14, 200);
    }
    if (ld16(p, 0x14) != 0) st16(p, 0x14, (uint16_t)(ld16(p, 0x14) - 1));
    /* (no floor on this divisor: retail) */
    int danger = 1000 * ld32(st, 0x54) / (ld32(st, 0x84) + ld32(st, 0x9c) + ld32(st, 0x58)) < 300;
    for (uint16_t k = 0; k < 18 && !danger; k++)
        if (ld16(st, 0x448u + 2u * k) != 0 && ld32(D(0x628760u), 140u * k) == 0) danger = 1;
    int32_t mood = danger ? 3 : world_scene_running() ? 2 : 1;
    if (mood != ld32(p, 0x18)) {
        if (ld32(p, 0x18) != 0) {
            st32(p, 0x1c, ld32(p, 0x18));
            st32(p, 0x20, 1000);
        }
        st32(p, 0x18, mood);
    }
    if (ld32(p, 0x20) > 0) st32(p, 0x20, dec_to0(ld32(p, 0x20), 40));
    for (uint16_t k = 0; k < 18; k++) hud_status_icon(p, k);
    for (uint16_t i = 0; (int32_t)i < ld32(p, 0x18c); i++) {
        uint8_t *e = p + 20u * i;
        st32(e, 0x34, dec_to0(ld32(e, 0x34), 20));
        if (ld32(e, 0x30) != 0) {
            int32_t a = dec_to0(ld32(e, 0x2c), 20);
            st32(e, 0x2c, a);
            if (a == 0) hud_status_drop(p, i);
        } else {
            st32(e, 0x2c, inc_to1000(ld32(e, 0x2c), 50));
        }
    }
}

int32_t hud_view_tick(uint8_t *v)
{
    if (ld32(v, 8) == 0) return 0;
    uint8_t *room = ldp(v, 4);
    uint8_t *leader = NULL;
    if (ldp(room, 0x200c) != NULL) leader = ldp(ldp(room, 0x200c), 0x9f4);
    if (ldp(v, 0x24) != NULL) panel_tick(v + 0x20);
    if (ldp(v, 0x1b4) != NULL) panel_tick(v + 0x1b0);
    st32(v, 0x3d0, (ld32(v, 0x3d0) + 1) % 256);
    {   /* the gold shown rolls toward the room's */
        int32_t g = ld32(room, 0x4014), s = ld32(v, 0x3d4), d = abs(g - s), step = 10;
        if (d <= 200) step = 1;
        if (d > 2000) step = 100;
        if (d > 20000) step = 1000;
        if (s < g) {
            s += step;
            st32(v, 0x3d4, g < s ? g : s);
        }
        if (ld32(v, 0x3d4) > g) {
            int32_t t = ld32(v, 0x3d4) - step;
            st32(v, 0x3d4, g <= t ? t : g);
        }
    }
    if (ld32(v, 0) == 0) {   /* hidden: everything fades */
        stp(v, 0x18, NULL);
        st32(v, 0x3c8, dec_to0(ld32(v, 0x3c8), 20));
        st32(v, 0x14, dec_to0(ld32(v, 0x14), 20));
        st32(v, 0x20, dec_to0(ld32(v, 0x20), 50));
        st32(v, 0x388, dec_to0(ld32(v, 0x388), 20));
        st32(v, 0x3d8, dec_to0(ld32(v, 0x3d8), 20));
        goto previous;
    }
    st32(v, 0x3c8, inc_to1000(ld32(v, 0x3c8), 20));
    if (leader == ldp(v, 0x24)) {
        st32(v, 0x20, inc_to1000(ld32(v, 0x20), 50));
    } else {   /* a new leader: the old panel cross-fades out */
        if (ldp(v, 0x24) != NULL) {
            memcpy(v + 0x1b0, v + 0x20, 0x190);
            st32(v, 0x1b0, 1000);
        }
        st32(v, 0x20, 0);
        stp(v, 0x24, leader);
        if (leader != NULL) {
            const uint8_t *st = ldp(leader, 0x750);
            st32(v, 0x28, hp_ratio(st));
            st32(v, 0x2c, stats_mp_ratio(st));
        } else {
            st32(v, 0x28, 0);
            st32(v, 0x2c, 0);
        }
        st16(v, 0x34, 0);
        st16(v, 0x30, 255);
        st16(v, 0x32, 255);
        st32(v, 0x38, 0);
        st32(v, 0x40, 0);
        st32(v, 0x1ac, 0);
        stp(v, 0x18, NULL);
        if (leader != NULL && ld32(leader, 0x200) == 0) {
            st16(v, 0x1c, 100);
            stp(v, 0x18, leader);
        }
    }
    if (ldp(v, 0x18) != NULL) {
        if (ld16(v, 0x1c) != 0) st16(v, 0x1c, (uint16_t)(ld16(v, 0x1c) - 1));
        else stp(v, 0x18, NULL);
    }
    st32(v, 0x14, ld16(room, 0x4070) != 0 ? inc_to1000(ld32(v, 0x14), 40) : dec_to0(ld32(v, 0x14), 20));
    if (ldp(v, 0x398) != NULL) {   /* the foe gauge */
        uint8_t *foe = ldp(v, 0x398);
        const wl_body *b = ((const wl_entity *)foe)->body;
        if (view_box_visible(g_world->view, b->x, b->y, b->w, b->h, 0)) {
            if (ld16(v, 0x3c4) != 0) st16(v, 0x3c4, (uint16_t)(ld16(v, 0x3c4) - 1));
        } else {
            st16(v, 0x3c4, 100);
        }
        int32_t c = ease(ld32(v, 0x390), hp_ratio(ldp(ldp(v, 0x398), 0x750)));
        st32(v, 0x390, c > 1000 ? 1000 : c);
        foe = ldp(v, 0x398);
        b = ((const wl_entity *)foe)->body;
        int keep = 0;
        if (ld32(foe, 0x1d0) == 1 && b->active == 1 && b->f7e == 0 && ld16(ldp(foe, 0x750), 0x44a) == 0) {
            int32_t kind = ld32(v, 0x394), rx, ry;
            if (kind == 10 || kind == 40 || kind == 43) { rx = 64000; ry = 32000; }
            else { rx = 32000; ry = 24000; }
            const camera_view *cv = g_world->view;
            if (abs(b->x + (b->w / 2 - cv->vp_w / 2 - cv->cur_x - cv->shake_x.out)) <= rx) {
                int32_t base = cv->cur_y + 100 * cv->y_off + cv->shake_y.out;
                int32_t cy = part_center_y((const wl_entity *)foe, 0);
                if (abs(cy - g_world->view->vp_h / 2 - base) <= ry) keep = 1;
            }
        }
        if (keep) {
            st32(v, 0x38c, 1000);
            st32(v, 0x388, inc_to1000(ld32(v, 0x388), 40));
        } else {
            stp(v, 0x398, NULL);
        }
    }
    if (ldp(v, 0x398) == NULL) {   /* ... none: it drains and fades */
        int32_t c = ld32(v, 0x390), d = abs(c) / 10;
        if (d < 5) d = 5;
        if (c < 0) {
            c += d;
            if (c > 0) c = 0;
        } else if (c > 0) {
            c -= d;
            if (c < 0) c = 0;
        }
        st32(v, 0x390, c > 1000 ? 1000 : c);
        if (ld32(v, 0x38c) > 0) {
            st32(v, 0x38c, ld32(v, 0x38c) - 5);
            st32(v, 0x388, inc_to1000(ld32(v, 0x388), 40));
        } else {
            v[0x39c] = 0;
            st32(v, 0x388, dec_to0(ld32(v, 0x388), 10));
        }
    }
    {   /* the hit combo */
        if (ld32(v, 0x3d8) > 0) {
            int32_t t = (uint32_t)ld32(room, 0x40e0) >= 5 ? ld32(room, 0x40e8) : 0;
            int32_t c = ease(ld32(v, 0x3dc), t);
            st32(v, 0x3dc, c > 2000 ? 2000 : c);
        } else {
            st32(v, 0x3dc, 0);
            st32(v, 0x3e4, 0);
            st32(v, 0x3e8, 0);
            st32(v, 0x3e0, 0);
        }
        uint32_t n = (uint32_t)ld32(room, 0x40e0), c = (uint32_t)ld32(v, 0x3e0);
        if (c < n) st32(v, 0x3e0, (int32_t)(n < c + 1 ? n : c + 1));
        c = (uint32_t)ld32(v, 0x3e0);
        if (c > n) st32(v, 0x3e0, (int32_t)(n > c - 1 ? n : c - 1));
        if ((uint32_t)ld32(room, 0x40e0) >= 5 && ld32(room, 0x40e8) > 0) {
            st32(v, 0x3e4, ld32(v, 0x3dc));
            st32(v, 0x3e8, ld32(room, 0x40e0));
            st32(v, 0x3d8, inc_to1000(ld32(v, 0x3d8), 40));
        } else if (ld32(v, 0x38c) > 0 && ld32(v, 0x3d8) > 0) {
            st32(v, 0x3d8, inc_to1000(ld32(v, 0x3d8), 40));
        } else {
            st32(v, 0x3d8, dec_to0(ld32(v, 0x3d8), 10));
        }
    }
previous:
    if (ldp(v, 0x1b4) != NULL) {   /* the previous leader's panel fades out, then resets */
        int32_t a = dec_to0(ld32(v, 0x1b0), 20);
        st32(v, 0x1b0, a);
        if (a == 0) {
            st32(v, 0x1b0, 0);
            stp(v, 0x1b4, NULL);
            st32(v, 0x1b8, 0);
            st32(v, 0x1bc, 0);
            st16(v, 0x1c0, 255);
            st16(v, 0x1c2, 255);
            st16(v, 0x1c4, 0);
            st32(v, 0x1c8, 0);
            st32(v, 0x1d0, 0);
            st32(v, 0x33c, 0);
        }
    }
    for (uint32_t k = 0; k < 8; k++) {   /* the party bars */
        const uint8_t *s = ldp(room, 0x4030u + 4u * k);
        if (ld32(s, 0x9c4) != 1 || ld32(s, 0x9f0) == 100000104) continue;
        const uint8_t *st = ldp(ldp(s, 0x9f4), 0x750);
        int32_t c = ease(ld32(v, 0x340u + 8u * k), hp_ratio(st));
        st32(v, 0x340u + 8u * k, c > 1000 ? 1000 : c);
        c = ease(ld32(v, 0x344u + 8u * k), stats_mp_ratio(st));
        st32(v, 0x344u + 8u * k, c > 1000 ? 1000 : c);
    }
    {   /* the bounce + the 3-step clock */
        int32_t pos = ld32(v, 0x380);
        if (pos <= 0) st32(v, 0x384, 100);
        int32_t spd = ld32(v, 0x384);
        pos += spd;
        st32(v, 0x384, spd - 5);
        st32(v, 0x380, pos < 0 ? 0 : pos);
        int32_t t = ld16(v, 0x3cc) + 3;
        st16(v, 0x3cc, (uint16_t)(t % 100));
        return t / 100;
    }
}
