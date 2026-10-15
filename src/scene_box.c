/* scene_box.c — the cutscene message box.  See scene_box.h.  Transcribed from the asm. */
#include "scene_box.h"

#include <stddef.h>
#include <string.h>

#include "camera_follow.h"
#include "engine_calls.h"
#include "ui_core.h"

static int32_t  ld32(const void *p, uint32_t off) { int32_t v; memcpy(&v, (const uint8_t *)p + off, 4); return v; }
static uint16_t ld16(const void *p, uint32_t off) { uint16_t v; memcpy(&v, (const uint8_t *)p + off, 2); return v; }
static void st32(void *p, uint32_t off, int32_t v) { memcpy((uint8_t *)p + off, &v, 4); }
static void st16(void *p, uint32_t off, uint16_t v) { memcpy((uint8_t *)p + off, &v, 2); }
static void *ldp(const void *p, uint32_t off) { void *v; memcpy(&v, (const uint8_t *)p + off, sizeof v); return v; }
static void stp(void *p, uint32_t off, const void *v) { memcpy((uint8_t *)p + off, &v, sizeof v); }

int32_t box_place(int32_t a1, uint8_t *win, int32_t a3, uint8_t *ta, uint8_t *tb, const wl_entity *who,
                  int32_t single, int32_t high)
{
    (void)a1; (void)a3;
    int32_t bw, bh = ld32(win, 0x18), x, y, tail, below = 0;
    if (who != NULL) {
        bw = ld32(win, 0x14);
        int32_t top = high != 0 ? 64 : 24;
        const wl_body *b = who->body;
        int32_t sx, sy;
        view_project((*g_eng.world)->view, b->x, b->y, 0, 0, 0, 0, &sx, &sy, 1);
        int32_t c = b->w / 100 / 2 - bw / 2 + sx;   /* centred on the speaker */
        int32_t m = 608 - bw < c ? 608 - bw : c;
        x = m < 32 ? 32 : m;
        y = b->margin / 100 + who->f1c - bh + sy - 48;
        int32_t t = b->w / 100 / 2 - x + sx;        /* the tail under the speaker's centre */
        int32_t tl = t < 32 ? 32 : t;
        tail = bw - 32 < tl ? bw - 32 : tl;
        if (y < top) {
            y = b->h / 100 + who->f20 + sy;
            below = 1;
        }
    } else {
        bw = ld32(win, 0x14);
        y = 80;
        x = (640 - bw) / 2;
        tail = bw / 2;
    }
    st32(win, 0xc, x);
    st32(win, 0x10, y);
    if (who == NULL) return tail;
    if (single != 0) {
        st32(tb, 0xc, tail - 16);
        st32(tb, 0x10, below ? -32 : bh);
        return (int32_t)cell_icon_set(tb, 0, 0, ld32(g_eng.sys, 0xba8), below ? 14 : 13, NULL, 0, 0);
    }
    if (below) {
        st32(ta, 0xc, tail - 16);
        st32(ta, 0x10, 0);
        cell_icon_set(ta, 0, 0, ld32(g_eng.sys, 0xba8), 11, NULL, 0, 0);
        st32(tb, 0xc, tail - 16);
        st32(tb, 0x10, -32);
        return (int32_t)cell_icon_set(tb, 0, 0, ld32(g_eng.sys, 0xba8), 12, NULL, 0, 0);
    }
    st32(ta, 0xc, tail - 16);
    st32(ta, 0x10, bh - 32);
    cell_icon_set(ta, 0, 0, ld32(g_eng.sys, 0xba8), 9, NULL, 0, 0);
    st32(tb, 0xc, tail - 16);
    st32(tb, 0x10, bh);
    /* (retail inlines this one: the same stores) */
    return (int32_t)cell_icon_set(tb, 0, 0, ld32(g_eng.sys, 0xba8), 10, NULL, 0, 0);
}

/* the alpha of fade step i (0..19; 20 and up: 0 = opaque) */
static int32_t fade_alpha(int32_t i)
{
    if (i < 0) i = 0;
    if (i >= 20) return 0;
    return ld32(g_eng.sys, 0x27a8u + 4u * (uint32_t)i);
}

int32_t box_fade_tick(uint8_t *bx)
{
    const uint8_t *win = ldp(bx, 0);
    if (ld32(win, 0x54) != 1000 || ld32(win, 4) == 0) return (int32_t)(intptr_t)win;
    uint16_t st = ld16(bx, 0x2e);
    uint8_t *ta = ldp(bx, 0x10), *tb = ldp(bx, 0x14);   /* the portrait now / before */
    int32_t t = ld32(bx, 0x30);
    if (st == 1) {   /* fading in: the new portrait over the first half, the old one out over the second */
        if (t >= 1000) {
            cell_icon_alpha(ta, 0, 0, 0);
            cell_icon_set(tb, 0, 0, 0, 0, NULL, 0, 0);
            st16(bx, 0x2e, 0);
            return 0;
        }
        if (t > 500) cell_icon_alpha(tb, 0, 0, fade_alpha(20 * (1000 - t) / 500));
        else cell_icon_alpha(ta, 0, 0, fade_alpha(20 * t / 500));
        t += ld32(bx, 0x34);
        if (t > 1000) t = 1000;
        st32(bx, 0x30, t);
        return t;
    }
    if (st != 2 && st != 3) return st;
    if (t > 0) {   /* fading out */
        cell_icon_alpha(ta, 0, 0, fade_alpha(20 * t / 1000));
        t -= ld32(bx, 0x34);
        if (t < 0) t = 0;
        st32(bx, 0x30, t);
        return t;
    }
    const uint8_t *gi = ldp(ta, 0x174);
    if (ld32(gi, 8) > 0 && ld32(gi, 0x10) > 0) {
        uint8_t *cells = ldp(ldp(ta, 0x17c), 0xc);
        if (ldp(cells, 4) == NULL) stp(cells, 4, g_eng.op_new(0x54));
        icon_set(ldp(ldp(ldp(ta, 0x17c), 0xc), 4), 0, 0, NULL, 0, 0);
    }
    st16(bx, 0x2e, 0);
    return 0;
}
