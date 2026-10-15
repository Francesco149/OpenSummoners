/* ui_core.c — the UI toolkit core.  See ui_core.h.
 *
 * Transcribed from the asm (these classes' IDA field names agree with it).  Allocations go through the
 * engine's operator new (g_eng.op_new) in retail's order, so the heap matches. */
#include "ui_core.h"

#include <stddef.h>
#include <string.h>

#include "engine_calls.h"
#include "room_load.h"

static int32_t  ld32(const void *p, uint32_t off) { int32_t v; memcpy(&v, (const uint8_t *)p + off, 4); return v; }
static uint16_t ld16(const void *p, uint32_t off) { uint16_t v; memcpy(&v, (const uint8_t *)p + off, 2); return v; }
static void st32(void *p, uint32_t off, int32_t v) { memcpy((uint8_t *)p + off, &v, 4); }
static void st16(void *p, uint32_t off, uint16_t v) { memcpy((uint8_t *)p + off, &v, 2); }
static void *ldp(const void *p, uint32_t off) { void *v; memcpy(&v, (const uint8_t *)p + off, sizeof v); return v; }
static void stp(void *p, uint32_t off, const void *v) { memcpy((uint8_t *)p + off, &v, sizeof v); }
static const void *D(uint32_t va) { return g_eng.data(va); }

void win_mgr_topmost(uint8_t *mgr)
{
    for (uint16_t i = 0; i < ld16(mgr, 6); i++)
        st32(((uint8_t **)ldp(mgr, 0))[i], 8, i == ld16(mgr, 6) - 1);
}

uint8_t *win_mgr_open(uint8_t *mgr, int32_t x, int32_t y, int32_t w, int32_t h, uint16_t cap, const void *style)
{
    uint16_t n = ld16(mgr, 6);
    if (n >= ld16(mgr, 4)) return NULL;
    win_init(((uint8_t **)ldp(mgr, 0))[n], mgr, x, y, w, h, cap, style);
    st16(mgr, 6, (uint16_t)(n + 1));
    win_mgr_topmost(mgr);
    return ((uint8_t **)ldp(mgr, 0))[n];
}

int win_init(uint8_t *win, uint8_t *mgr, int32_t x, int32_t y, int32_t w, int32_t h, uint16_t cap, const void *style)
{
    if (style != NULL) memcpy(win + 0x5c, style, 0x24);
    else st32(win, 0x5c, 0);
    stp(win, 0, mgr);
    st32(win, 0xc, x);
    st32(win, 0x14, w);
    st32(win, 4, 1);
    st32(win, 8, 0);
    st32(win, 0x80, 0);
    st32(win, 0x10, y);
    st32(win, 0x18, h);
    if (ldp(win, 0x48) != NULL) {
        for (uint16_t i = 0; i < ld16(win, 0x4c); i++) {
            uint8_t **list = ldp(win, 0x48);
            uint8_t *c = list[i];
            if (c == NULL) continue;
            ctrl_geometry_free(c);
            g_eng.op_delete(c);
            ((uint8_t **)ldp(win, 0x48))[i] = NULL;
        }
        if (ldp(win, 0x48) != NULL) {
            g_eng.op_delete(ldp(win, 0x48));
            stp(win, 0x48, NULL);
        }
    }
    st16(win, 0x4c, cap);
    stp(win, 0x48, g_eng.op_new(4u * cap));
    for (uint16_t i = 0; i < ld16(win, 0x4c); i++) {
        uint8_t *c = g_eng.op_new(0x1b0);
        if (c != NULL) {
            st32(c, 0x170, 0); st32(c, 0x164, 0); st32(c, 0x174, 0); st32(c, 0x178, 0); st32(c, 0x17c, 0);
            st32(c, 0x180, 0x3e537d);
            stp(c, 0x188, D(0x677b98u));
            st32(c, 0x184, 0xa8b9cc);
            st32(c, 0x18c, 0xf08080);
            st32(c, 0x190, 0xf08080);
            stp(c, 0x194, D(0x8090a9u));
            stp(c, 0x198, D(0x8090a9u));
            st32(c, 0x19c, 0x3e537d);
            st32(c, 0x1a0, 0xa8b9cc);
            st32(c, 0x1a4, 0); st32(c, 0x1a8, 0); st32(c, 0x14, 0); st32(c, 0x18, 0);
            st32(c, 0x1ac, 28);
        }
        ((uint8_t **)ldp(win, 0x48))[i] = c;
    }
    st16(win, 0x4e, 0);
    st32(win, 0x54, 0);
    st32(win, 0x58, 0);
    st32(win, 0x1c, 1);
    st32(win, 0x50, 1);
    return 1;
}

uint8_t *win_ctrl_take(uint8_t *win)
{
    uint16_t n = ld16(win, 0x4e);
    if (ld16(win, 0x4c) <= n) return NULL;
    uint8_t *c = ((uint8_t **)ldp(win, 0x48))[n];
    stp(c, 0, win);
    st16(c, 4, n);
    st32(c, 8, 0);
    st16(win, 0x4e, (uint16_t)(n + 1));
    return c;
}

uint8_t *win_ctrl_at(const uint8_t *win, uint16_t i)
{
    return i < ld16(win, 0x4e) ? ((uint8_t **)ldp(win, 0x48))[i] : NULL;
}

int32_t grid_col_place(uint8_t *c, int32_t col, int32_t x, int32_t a, int32_t w, int32_t b)
{
    uint8_t *t = (uint8_t *)ldp(c, 0x178) + 36 * col;
    st32(t, 0, x);
    st32(t, 4, a);
    st32(t, 8, w);
    st32(t, 0xc, b);
    return 36 * col;
}

int32_t grid_col_style(uint8_t *c, int32_t col, int32_t a, int32_t b, int32_t d, int32_t e, int32_t f)
{
    uint8_t *t = (uint8_t *)ldp(c, 0x178) + 36 * col;
    st32(t, 0x10, a);
    st32(t, 0x14, b);
    st32(t, 0x18, d);
    st32(t, 0x1c, e);
    st32(t, 0x20, f);
    return 36 * col;
}

int32_t ui_ref_set(uint8_t *r, int32_t a, uint16_t b)
{
    st32(r, 0, a);
    st16(r, 4, b);
    st32(r, 8, 0);
    return a;
}

uint8_t *ui_link_drop(uint8_t *o)
{
    uint8_t *l = ldp(o, 0x20);
    if (l == NULL) return NULL;
    st32(l, 0x50, 0);
    st32(o, 0x20, 0);
    st32(o, 0x24, 0);
    return l;
}

uint8_t *ctrl_text_init(uint8_t *c, int32_t x, int32_t y, uint16_t w, uint16_t h)
{
    ctrl_geometry_free(c);
    st32(c, 0xc, x);
    st32(c, 0x10, y);
    st32(c, 8, 2);
    st32(c, 0x20, 0);
    st32(c, 0x84, 0);
    st32(c, 0x140, 0);
    stp(c, 0x170, g_eng.op_new(0x2c));
    uint8_t *g = g_eng.op_new(0x10);   /* the glyph buffer {+0 positions 512 x 12, u16 +4 cap, +8 512 x 2, +0xc} */
    if (g != NULL) {
        st32(g, 0xc, 0);
        st32(g, 0, 0);
        st32(g, 8, 0);
        uint8_t *b = g_eng.op_new(0xc);   /* {+0 the 512 x 0x24 glyph bytes, +4 cap, u16 +8, u16 +0xa} */
        if (b != NULL) {
            st32(b, 4, 0x200);
            stp(b, 0, g_eng.op_new(0x4800));
            st16(b, 8, 0);
            st16(b, 0xa, 0);
        }
        stp(g, 0xc, b);
        stp(g, 0, g_eng.op_new(0x1800));
        stp(g, 8, g_eng.op_new(0x400));
        st16(g, 4, 0x200);
    }
    uint8_t *t = ldp(c, 0x170);
    stp(t, 0, g);
    st16(t, 4, 0);
    st16(t, 6, 0);
    st16(t, 8, w);
    st16(t, 0xa, h);
    st32(t, 0x1c, 0); st32(t, 0xc, 0); st32(t, 0x10, 0); st32(t, 0x20, 0); st32(t, 0x24, 0); st32(t, 0x28, 0);
    st32(t, 0x14, 0); st32(t, 0x18, 0);
    st32(c, 0x1c, 0);
    return t;
}

uint8_t *win_text_add(uint8_t *win, int32_t x, int32_t y, uint16_t w, uint16_t h)
{
    uint8_t *c = win_ctrl_take(win);
    if (c == NULL) return NULL;
    ctrl_text_init(c, x, y, w, h);
    return c;
}

int32_t grid_row_reset(uint8_t *c, int32_t row)
{
    for (uint16_t k = 0; (int32_t)k < ld32(ldp(c, 0x174), 8); k++) {
        uint8_t *cells = ldp((uint8_t *)ldp(c, 0x17c) + 16 * row, 0xc);
        if (ld32(cells, 24u * k) != 0) g_eng.h40fa00(c, row, k, (intptr_t)D(0x8a9b6cu));
        cells = ldp((uint8_t *)ldp(c, 0x17c) + 16 * row, 0xc);
        const uint8_t *gi = ldp(c, 0x174);
        if (ldp(cells, 24u * k + 4) != NULL && ld32(gi, 8) > k && ld32(gi, 0x10) > row) {
            if (ldp(cells, 24u * k + 4) == NULL) stp(cells, 24u * k + 4, g_eng.op_new(0x54));
            uint8_t *ic = ldp(ldp((uint8_t *)ldp(c, 0x17c) + 16 * row, 0xc), 24u * k + 4);
            st32(ic, 0, 0); st16(ic, 4, 0); st16(ic, 0x46, 0); st16(ic, 0x48, 0); st16(ic, 0x4c, 0); st16(ic, 0x4a, 0);
            st32(ic, 0x50, 0);
        }
        cells = ldp((uint8_t *)ldp(c, 0x17c) + 16 * row, 0xc);
        gi = ldp(c, 0x174);
        if (ldp(cells, 24u * k + 8) != NULL && ld32(gi, 8) > k && ld32(gi, 0x10) > row) {
            if (ldp(cells, 24u * k + 8) == NULL) stp(cells, 24u * k + 8, g_eng.op_new(0x20));
            uint8_t *gg = ldp(ldp((uint8_t *)ldp(c, 0x17c) + 16 * row, 0xc), 24u * k + 8);
            st32(gg, 4, 0); st16(gg, 8, 0); st32(gg, 0, 0); st32(gg, 0xc, 0); st32(gg, 0x10, 0); st32(gg, 0x14, 0);
            st32(gg, 0x18, 0);
            gi = ldp(c, 0x174);
            if (ld32(gi, 8) > k && ld32(gi, 0x10) > row) {
                gg = ldp(ldp((uint8_t *)ldp(c, 0x17c) + 16 * row, 0xc), 24u * k + 8);
                if (gg != NULL) {
                    int32_t lo = ld32(gg, 0x18) < 0 ? ld32(gg, 0x18) : 0, v = ld32(gg, 0x14);
                    st32(gg, 0x1c, v > lo ? v : lo);
                }
            }
        }
    }
    /* (retail's eax: the column count, or the grid record itself when there are none) */
    return ld32(ldp(c, 0x174), 8) > 0 ? ld32(ldp(c, 0x174), 8) : (int32_t)(intptr_t)ldp(c, 0x174);
}

uint8_t *win_grid_add(uint8_t *win, int32_t x, int32_t y, int32_t w, int32_t h)
{
    uint8_t *c = win_ctrl_take(win);
    if (c == NULL) return NULL;
    ctrl_geometry_free(c);
    st32(c, 0xc, x);
    st32(c, 0x10, y);
    st32(c, 8, 1);
    st32(c, 0x20, 0);
    st32(c, 0x84, 0);
    st32(c, 0x140, 0);
    uint8_t *gi = g_eng.op_new(0x24);   /* {1, rows, cols, page, used rows, cursor, page top, ...} */
    stp(c, 0x174, gi);
    st32(gi, 0x18, 0); st32(gi, 0x14, 0); st32(gi, 4, 1); st32(gi, 8, 1); st32(gi, 0xc, 1); st32(gi, 0, 1);
    st32(gi, 0x1c, 0); st32(gi, 0x20, 0); st32(gi, 0x10, 0);
    stp(c, 0x17c, g_eng.op_new(16u * (uint32_t)ld32(gi, 4)));
    stp(c, 0x178, g_eng.op_new(36u * (uint32_t)ld32(gi, 8)));
    for (uint16_t r = 0; (int32_t)r < ld32(ldp(c, 0x174), 4); r++) {
        uint8_t *row = (uint8_t *)ldp(c, 0x17c) + 16u * r;
        st32(row, 0, 0);
        st32(row, 4, 0);
        stp(row, 0xc, g_eng.op_new(24u * (uint32_t)ld32(ldp(c, 0x174), 8)));
        for (uint16_t k = 0; (int32_t)k < ld32(ldp(c, 0x174), 8); k++) {
            uint8_t *cell = (uint8_t *)ldp((uint8_t *)ldp(c, 0x17c) + 16u * r, 0xc) + 24u * k;
            st32(cell, 0, 0); st32(cell, 4, 0); st32(cell, 8, 0); st32(cell, 0xc, 0);
        }
    }
    for (uint16_t k = 0; (int32_t)k < ld32(ldp(c, 0x174), 8); k++) {
        grid_col_place(c, k, 32 * k, 0, 32, 0);
        grid_col_style(c, k, 0, 0, 0, 0, 0);
    }
    uint8_t *c0 = ldp(c, 0x178);
    st32(c0, 0, 0);
    st32(c0, 4, 0);
    st32(c0, 8, w);
    st32(c0, 0xc, h);
    gi = ldp(c, 0x174);
    int32_t used = ld32(gi, 0x10);
    if (used < ld32(gi, 4)) {
        st32(gi, 0x10, used + 1);
        uint8_t *row = (uint8_t *)ldp(c, 0x17c) + 16 * used;
        st32(row, 0, 0);
        st32(row, 4, 0);
        st32(row, 8, 1);
        grid_row_reset(c, used);
    }
    return c;
}

uint8_t *text_metrics_set(uint8_t *c, int32_t a, int32_t line_h, int32_t d, int32_t e)
{
    uint8_t *t = ldp(c, 0x170);
    st32(t, 0xc, a);
    st32(t, 0x10, line_h);
    st32(t, 0x20, 2 * line_h);
    st32(t, 0x24, 3 * line_h);
    st32(t, 0x28, 5 * line_h);
    st32(t, 0x14, d);
    st32(t, 0x18, e);
    st32(c, 0x1c, 0);
    return t;
}

int32_t ctrl_cursor_set(uint8_t *c, int32_t sprite, uint16_t size, const uint16_t *frames, uint16_t n,
                        uint16_t rate, int32_t dx, int32_t dy)
{
    st32(c, 0x20, 1);
    st16(c, 0x2c, size);
    st32(c, 0x28, sprite);
    st16(c, 0x6e, n);
    if (frames != NULL && n != 0) memcpy(c + 0x2e, frames, 2u * n);
    st16(c, 0x70, rate);
    st16(c, 0x74, 0);
    st16(c, 0x72, 0);
    st32(c, 0x78, 0);
    st32(c, 0x24, 0);
    st32(c, 0x7c, dx);
    st32(c, 0x80, dy);
    return dy;
}

int32_t icon_set(uint8_t *rec, int32_t sprite, uint16_t a3, const uint16_t *frames, uint16_t n, uint16_t a6)
{
    st16(rec, 4, a3);
    st32(rec, 0, sprite);
    st16(rec, 0x46, n);
    if (frames != NULL && n != 0) memcpy(rec + 6, frames, 2u * n);
    st16(rec, 0x4c, 0);
    st16(rec, 0x48, a6);
    st16(rec, 0x4a, 0);
    st32(rec, 0x50, 0);
    return n;
}

int grid_page_fit(uint8_t *c)
{
    uint8_t *gi = ldp(c, 0x174);
    int32_t page = ld32(gi, 0xc), cur = ld32(gi, 0x14), top = 0;
    while (cur < top || cur >= page + top) top += page;   /* (a negative cursor never ends, as in retail) */
    if (top == ld32(gi, 0x18)) return 0;
    st32(gi, 0x18, top);
    return 1;
}

int32_t ui_scroll_init(uint8_t *s, int32_t a2, int32_t a3, int32_t a4, int32_t a5)
{
    st32(s, 0x1c, 0); st32(s, 0x20, 0); st32(s, 0x24, 0); st32(s, 0x28, 0);
    st16(s, 0x18, 0);
    st32(s, 0x30, 0);
    st32(s, 4, a2);
    st32(s, 0, a3);
    st32(s, 8, a5);
    st32(s, 0xc, a4);
    st32(s, 0x10, 1);
    return a4;
}

uint8_t *sys_frame_style(void *sys, uint8_t *style, int32_t alt)
{
    st32(style, 0x18, 32);
    st32(style, 0x1c, 32);
    st16(style, 4, 0);
    static const uint16_t base[2][3] = { { 0, 1, 2 }, { 9, 10, 11 } };
    for (int i = 0; i < 3; i++) st16(style, 6 + 2 * i, base[alt != 0][i]);
    for (uint16_t i = 0; i < 6; i++) st16(style, 0xc + 2u * i, (uint16_t)(3 + i));
    st32(style, 0x20, 0);
    st32(style, 0, ld32(sys, 0xb88));
    return style;
}

uint8_t *sys_cursor_style(void *sys, uint8_t *c, int32_t on)
{
    if (on != 0) {   /* (0x411ec0 inlined: the 4-frame cursor) */
        static const uint16_t frames[4] = { 0, 1, 2, 3 };
        st32(c, 0x20, 1);
        st32(c, 0x28, ld32(sys, 0xb8c));
        st16(c, 0x2c, 16);
        st16(c, 0x6e, 4);
        memcpy(c + 0x2e, frames, sizeof frames);
        st16(c, 0x70, 16);
        st16(c, 0x74, 0);
        st16(c, 0x72, 0);
        st32(c, 0x78, 0);
        st32(c, 0x7c, -32);
        st32(c, 0x80, -30);
        st32(c, 0x24, 0);
    }
    const uint8_t *cfg = (const uint8_t *)(intptr_t)ld32((const void *)(intptr_t)ld32(sys, 0x320), 0);
    if (ld32(cfg, 0x254) == 0) {
        st32(c, 0x14, 1);
        st32(c, 0x18, -9);
    }
    return c;
}

int32_t money_value(int32_t kind, int32_t v, int32_t up)
{
    if (kind != 10) return v;
    if (up != 0) v += 99;
    return v / 100;
}

char *money_text(char *buf, int32_t kind, int32_t v, int32_t up)
{
    if (kind == 10) g_eng.h5bf3ee(buf, (const char *)D(0x854a34u), money_value(kind, v, up));   /* "%dGP" */
    else g_eng.h5bf3ee(buf, (const char *)D(0x854a20u), v);                                    /* "%dCP" */
    return buf;
}

int32_t room_item_count(const wl_room *r, int32_t item, uint16_t *best, int32_t any)
{
    int32_t n = 0, pick = -1;
    uint16_t low = 0;
    for (uint16_t i = 0; i < ld16(r, 0x4010); i++) {
        const uint8_t *rec = (const uint8_t *)r + 0x2010 + 16u * i;
        if (any == 0 && rec[0xa] != 0) continue;
        if (ld32(rec, 0) != item) continue;
        if (best != NULL && (pick == -1 || low > ld16(rec, 8))) { pick = i; low = ld16(rec, 8); }
        n++;
    }
    if (best != NULL && pick >= 0) *best = (uint16_t)pick;
    return n;
}

int32_t room_party_count(const wl_room *r, int32_t all, int32_t fit)
{
    int32_t n = 0;
    for (int k = 0; k < 8; k++) {
        const wl_pslot *s = r->slot[k];
        if (s->active != 1) continue;
        if (s->id == 100000104 && all == 0) continue;
        if (fit != 0) {
            const wl_char *c = s->ch;
            if (c->base.alive != 1 || c->base.body->active != 1 || ld16(c->party, 0x44a) != 0) continue;
        }
        n++;
    }
    return n;
}
