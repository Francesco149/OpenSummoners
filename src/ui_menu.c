/* ui_menu.c — the menu screens on the UI toolkit.  See ui_menu.h.  Transcribed from the asm. */
#include "ui_menu.h"

#include <stddef.h>
#include <string.h>

#include "engine_calls.h"
#include "room_loop.h"   /* pslot_ok 0x412db0 */
#include "ui_core.h"
#include "ui_text.h"

static int32_t  ld32(const void *p, uint32_t off) { int32_t v; memcpy(&v, (const uint8_t *)p + off, 4); return v; }
static uint16_t ld16(const void *p, uint32_t off) { uint16_t v; memcpy(&v, (const uint8_t *)p + off, 2); return v; }
static void st32(void *p, uint32_t off, int32_t v) { memcpy((uint8_t *)p + off, &v, 4); }
static void st16(void *p, uint32_t off, uint16_t v) { memcpy((uint8_t *)p + off, &v, 2); }
static void *ldp(const void *p, uint32_t off) { void *v; memcpy(&v, (const uint8_t *)p + off, sizeof v); return v; }
static void stp(void *p, uint32_t off, const void *v) { memcpy((uint8_t *)p + off, &v, sizeof v); }
static const void *D(uint32_t va) { return g_eng.data(va); }

/* the window's control `i` (NULL past the taken ones) */
static uint8_t *win_ctrl(const uint8_t *win, uint16_t i)
{
    return ld16(win, 0x4e) <= i ? NULL : ((uint8_t **)ldp(win, 0x48))[i];
}

uint8_t *menu_msg_open(uint8_t *m)
{
    static const uint16_t fr[10] = { 0, 0, 1, 2, 3, 4, 5, 6, 7, 8 };
    uint8_t st[0x24];
    st32(st, 0, ld32(g_eng.sys, 0xb88));
    memcpy(st + 4, fr, sizeof fr);
    st32(st, 0x18, 32);
    st32(st, 0x1c, 32);
    st32(st, 0x20, 0);
    uint8_t *win = win_mgr_open(ldp(m, 0), 8, 346, 624, 128, 1, st);
    stp(m, 0x20, win);
    uint8_t *c = win_text_add(win, 64, 24, 68, 3);
    stp(m, 0x24, c);
    sys_msg_keys(g_eng.sys, (int32_t)(intptr_t)ldp(m, 0x20), c);
    c = ldp(m, 0x24);
    uint8_t *t = ldp(c, 0x170);
    st32(t, 0xc, 0); st32(t, 0x10, 0); st32(t, 0x20, 0); st32(t, 0x24, 0); st32(t, 0x28, 0); st32(t, 0x14, 0);
    st32(t, 0x18, 0);
    st32(c, 0x1c, 0);
    return c;
}

void menu_party_open(uint8_t *m)
{
    int32_t n = 0;
    for (int i = 0; i < 8; i++)
        if (pslot_ok(ldp(ldp(m, 0xc), 0x4030u + 4u * (uint32_t)i), 0, 0)) n++;
    int32_t h = 28 * (n + 1);
    uint8_t st[0x24];
    sys_frame_style(g_eng.sys, st, 1);
    uint8_t *win = win_mgr_open(ldp(m, 0), 352, 8, 288, h + 104, 3, st);
    win_title_add(win, "Party Member Status");
    stp(m, 0x28, win);
    /* the members: name, LV, HP, / max, MP, / max */
    uint8_t *c = win_ctrl_take(win);
    if (c != NULL) ctrl_grid_init(c, 0, 40, 9, 6, 8, 1);
    static const int32_t cols[6][4] = { { 0, 0, 88, 1 }, { 96, 0, 32, 1 }, { 128, 0, 32, 1 }, { 160, 0, 0, 0 },
                                        { 192, 0, 32, 1 }, { 224, 0, 0, 0 } };
    for (int k = 0; k < 6; k++)
        for (int j = 0; j < 4; j++) st32(ldp(c, 0x178), 36u * (uint32_t)k + 4u * (uint32_t)j, cols[k][j]);
    /* the money: "Money", the amount, its cap */
    uint8_t *c2 = win_ctrl_take(ldp(m, 0x28));
    if (c2 != NULL) ctrl_grid_init(c2, 0, h + 56, 1, 3, 1, 1);
    grid_col_place(c2, 0, 0, 0, 88, 1);
    grid_col_place(c2, 1, 96, 0, 80, 1);
    grid_col_place(c2, 2, 176, 0, 96, 1);
    grid_row_add(c2, 0, 0);
    menu_party_fill(m);
}

void menu_party_fill(uint8_t *m)
{
    uint8_t *room = ldp(m, 0xc);
    if (ldp(m, 0x28) == NULL) return;
    uint8_t *c = win_ctrl(ldp(m, 0x28), 1);
    char buf[512];
    st32(ldp(c, 0x174), 0x10, 0);
    grid_row_add(c, 0, 0);
    grid_cell_text(c, 0, 0, D(0x8a9b6cu));
    grid_cell_text(c, 0, 1, "LV");
    grid_cell_text(c, 0, 2, "HP");
    grid_cell_text(c, 0, 4, "MP");
    for (uint32_t i = 0; i < 8; i++) {
        const uint8_t *s = ldp(room, 0x4030u + 4u * i);
        if (ld32(s, 0x9c4) != 1 || ld32(s, 0x9f0) == 100000104) continue;
        int32_t row = grid_row_add(c, 0, 0);
        grid_cell_text(c, row, 0, (const char *)s + 0xa10);
        g_eng.h5bf3ee(buf, "%d", ld32(s, 0xaf0) + ld32(s, 0xae8));
        grid_cell_text(c, row, 1, buf);
        g_eng.h5bf3ee(buf, "%d", ld32(s, 0xa64));
        grid_cell_text(c, row, 2, buf);
        g_eng.h5bf3ee(buf, "/ %d", ld32(s, 0xaac) + ld32(s, 0xa94) + ld32(s, 0xa68));
        grid_cell_text(c, row, 3, buf);
        g_eng.h5bf3ee(buf, "%d", ld32(s, 0xa6c));
        grid_cell_text(c, row, 4, buf);
        g_eng.h5bf3ee(buf, "/ %d", ld32(s, 0xab0) + ld32(s, 0xa98) + ld32(s, 0xa70));
        grid_cell_text(c, row, 5, buf);
    }
    uint8_t *c2 = win_ctrl(ldp(m, 0x28), 2);
    grid_cell_text(c2, 0, 0, "Money");
    int32_t gold = ld32(room, 0x4014);
    if (ld32(ldp(m, 0xc), 0x4064) == 10) g_eng.h5bf3ee(buf, "%dGP", gold / 100);
    else g_eng.h5bf3ee(buf, "%dCP", gold);
    grid_cell_text(c2, 0, 1, buf);
    int32_t cap = ld32(room, 0x4020);
    if (ld32(room, 0x4064) == 10) cap /= 100;
    g_eng.h5bf3ee(buf, "(Max %d)", cap);
    grid_cell_text(c2, 0, 2, buf);
}

intptr_t menu_have_counts(uint8_t *m, uint8_t *c)
{
    char buf[512];
    if (ld32(ldp(c, 0x174), 0x10) <= 0) return (intptr_t)ldp(c, 0x174);
    uint16_t r = 0;
    do {
        int32_t item = ld32((uint8_t *)ldp(c, 0x17c) + 16u * r, 4);
        const uint8_t *room = ldp(m, 0xc);
        uint16_t cnt = 0;
        for (uint16_t k = 0, n = ld16(room, 0x4010); k < n; k++) {
            const uint8_t *rec = room + 0x2010 + 16u * k;
            if (rec[0xa] == 0 && ld32(rec, 0) == item) cnt++;
        }
        if (cnt != 0) {
            g_eng.h5bf3ee(buf, "You have %d", cnt);
            grid_cell_text(c, r, 4, buf);
        } else {
            grid_cell_text(c, r, 4, D(0x8a9b6cu));
        }
        r++;
    } while ((int32_t)r < ld32(ldp(c, 0x174), 0x10));
    return ld32(ldp(c, 0x174), 0x10);
}

int32_t menu_item_fits(const uint8_t *m, int32_t code, const uint8_t *item)
{
    const uint8_t *room = ldp(m, 0xc), *s;
    for (uint16_t i = 0;; ) {
        s = ldp(room, 0x4030u + 4u * i);
        if (ld32(s, 0x9f0) == code) break;
        if (++i >= 8) return 0;
    }
    if ((ld32(s, 0x9c4) != 1 && ld32(s, 0x9c4) != 2) || s == NULL) return 0;
    int32_t kind = ld32(item, 0x7c);
    switch (ld32(s, 0x9f0)) {
    case 100000101: return kind == 1 || kind == 2 || kind == 4 || kind == 9;
    case 100000102: return kind == 1 || kind == 3 || kind == 5 || kind == 9;
    case 100000103: return kind == 1 || kind == 3 || kind == 6 || kind == 9;
    case 100000104: return kind == 8;
    default: return 0;
    }
}

int32_t menu_sprites_free(uint8_t *m)
{
    uint16_t idx = ld16(m, 0x18);
    if (idx == 0) return 0;
    uint8_t *set = ldp(g_eng.sys, 0xaacu + 4u * idx);
    for (uint16_t k = 0; k < ld16(set, 4); k++) {
        uint8_t *ents = ldp(set, 0);
        if (ents == NULL || ldp(ents + 8u * k, 0) == NULL) continue;
        uint8_t *e = ents + 8u * k;
        for (uint32_t i = 0; i < (uint32_t)ld32(set, 0x38); i++) {
            void *f = ((void **)ldp(e, 0))[i];
            if (f != NULL) {
                g_eng.h5b9390(f);
                g_eng.op_delete(f);
            }
            ((void **)ldp(e, 0))[i] = NULL;
        }
        g_eng.op_delete(ldp(e, 0));
        stp(e, 0, NULL);
    }
    st32(ldp(g_eng.sys, 0x18e0u + 4u * idx), 0x10, 0);
    st16(m, 0x18, 0);
    return idx;
}
