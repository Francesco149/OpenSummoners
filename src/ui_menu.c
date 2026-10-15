/* ui_menu.c — the menu screens on the UI toolkit.  See ui_menu.h.  Transcribed from the asm. */
#include "ui_menu.h"

#include <stddef.h>
#include <string.h>

#include "char_ai.h"     /* cmd_clear 0x411530 */
#include "engine_calls.h"
#include "gimmick.h"     /* room_input_flush 0x411560 */
#include "party_member.h"
#include "room_loop.h"   /* pslot_ok 0x412db0 */
#include "script_engine.h"
#include "sound_fx.h"
#include "sprite_slot.h"
#include "ui_core.h"
#include "ui_run.h"
#include "ui_text.h"

static int32_t  ld32(const void *p, uint32_t off) { int32_t v; memcpy(&v, (const uint8_t *)p + off, 4); return v; }
static uint16_t ld16(const void *p, uint32_t off) { uint16_t v; memcpy(&v, (const uint8_t *)p + off, 2); return v; }
static void st32(void *p, uint32_t off, int32_t v) { memcpy((uint8_t *)p + off, &v, 4); }
static void st16(void *p, uint32_t off, uint16_t v) { memcpy((uint8_t *)p + off, &v, 2); }
static void *ldp(const void *p, uint32_t off) { void *v; memcpy(&v, (const uint8_t *)p + off, sizeof v); return v; }
static void stp(void *p, uint32_t off, const void *v) { memcpy((uint8_t *)p + off, &v, sizeof v); }
static const void *D(uint32_t va) { return g_eng.data(va); }
static const char *S(uint32_t va) { return (const char *)g_eng.data(va); }

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

/* ── chip 3c: the shop (driver, buy list, "On who?" + the equip preview) ─────────────── */

/* the id (+4) of the grid's cursor row */
static int32_t cur_id(const uint8_t *c)
{
    return ld32((const uint8_t *)ldp(c, 0x17c) + 16u * (uint32_t)ld32(ldp(c, 0x174), 0x14), 4);
}

/* the cell's icon record (0x54 B, made on first use) when the cell exists */
static void cell_icon(uint8_t *c, int32_t col, int32_t sprite, uint16_t frame)
{
    const uint8_t *gi = ldp(c, 0x174);
    if (ld32(gi, 8) <= col || ld32(gi, 0x10) <= 0) return;
    uint8_t *cells = ldp(ldp(c, 0x17c), 0xc);
    if (ldp(cells, 24u * (uint32_t)col + 4) == NULL) stp(cells, 24u * (uint32_t)col + 4, g_eng.op_new(0x54));
    icon_set(ldp(ldp(ldp(c, 0x17c), 0xc), 24u * (uint32_t)col + 4), sprite, frame, NULL, 0, 0);
}

int32_t member_rows_add(const wl_room *r, uint8_t *c, int32_t all)
{
    for (int i = 0; i < 8; i++) {
        const uint8_t *s = (const uint8_t *)r->slot[i];
        if (ld32(s, 0x9c4) != 1) continue;
        int32_t id = ld32(s, 0x9f0);
        if (id == 100000104 && all == 0) continue;
        int32_t row = grid_row_add(c, 1, id);
        grid_cell_text(c, row, 0, (const char *)s + 0xa10);
    }
    return 0;
}

void stat_row_set(uint8_t *c, uint16_t row, const char *label, int32_t base, int32_t cur, int32_t next, int16_t icon)
{
    char buf[512];
    int32_t now = base + cur, then = base + next;
    cell_color_set(c, row, 1, 0, 0, 0);
    cell_color_set(c, row, 2, 0, 0, 0);
    cell_color_set(c, row, 3, 0, 0, 0);
    if (now == then) {
        cell_color_set(c, row, 4, 0, 0, 0);
    } else {
        int32_t col = now >= then ? 0xff : 0xff0000;
        cell_color_set(c, row, 4, 1, col, col);
    }
    int32_t spr = ld32(g_eng.sys, 0xb88);
    const uint8_t *gi = ldp(c, 0x174);
    if (ld32(gi, 8) > 0 && ld32(gi, 0x10) > (int32_t)row) {
        uint8_t *cells = ldp((uint8_t *)ldp(c, 0x17c) + 16u * row, 0xc);
        if (ldp(cells, 4) == NULL) stp(cells, 4, g_eng.op_new(0x54));
        icon_set(ldp(ldp((uint8_t *)ldp(c, 0x17c) + 16u * row, 0xc), 4), spr, (uint16_t)icon, NULL, 0, 0);
    }
    grid_cell_text(c, row, 1, label);
    g_eng.h5bf3ee(buf, "%d", now);
    grid_cell_text(c, row, 2, buf);
    grid_cell_text(c, row, 3, ">>");
    g_eng.h5bf3ee(buf, "%d", then);
    grid_cell_text(c, row, 4, buf);
}

void member_sprite(uint8_t *m, const uint8_t *s, uint16_t *bank, uint16_t *frame, const uint8_t **clip, int32_t alt,
                   const uint8_t *item)
{
    void *sys = g_eng.sys;
    const uint8_t *ch = ldp(s, 0x9f4);
    if (ch == NULL) {
        *bank = 0;
        return;
    }
    uint32_t k = ld32(ch, 0x1d4) == 50010 && alt != 0 ? 1 : 0;
    *bank = ld16(ch, 0x48 + 20 * k);
    *frame = ld16(ch, 0x4a + 20 * k);
    *clip = ldp(ch, 0x15914);
    g_eng.h417c40(sys, *bank, 0);
    if (item == NULL || ldp(item, 0x2b4) == NULL) return;
    uint16_t old = ld16(m, 0x18);
    if (old != 0) {
        ar_sprite_slot *sl = ldp(sys, 0xaacu + 4u * old);
        for (uint16_t e = 0; e < sl->entry_count; e++) slot_frames_free(sl, e);
        st32(ldp(sys, 0x18e0u + 4u * old), 0x10, 0);
        st16(m, 0x18, 0);
    }
    /* a free runtime bank (884..908) becomes a clone of the member's */
    uint16_t src = *bank, nb = 0;
    for (uint16_t b = 0x374; b <= 0x38c; b++) {
        uint8_t *d = ldp(sys, 0x18e0u + 4u * b);
        if (ld32(d, 0x10) != 0) continue;
        st32(d, 0x10, 1);
        slot_clone(sys, b, src);
        nb = b;
        break;
    }
    st16(m, 0x18, nb);
    if (nb == 0) return;
    /* its palette with the member's equipment swaps, then the previewed item's */
    uint8_t pal[1024] = { 0 };
    g_eng.h4178e0(ldp(sys, 0xaacu + 4u * nb), (intptr_t)pal);
    const uint8_t *room = ldp(s, 0x9ec);
    int32_t id = ld32(s, 0x9f0);
    for (uint16_t i = 0; i < ld16(room, 0x4010); i++) {
        const uint8_t *rec = room + 0x2010 + 16u * i;
        if (ld32(rec, 4) != id) continue;
        const uint8_t *d = ldp(rec, 0xc);
        if (ld32(d, 0x78) == 0 || ldp(d, 0x2b4) == NULL) continue;
        pal_swap_apply(sys, ldp(d, 0x2b4), pal);
    }
    pal_swap_apply(sys, ldp(item, 0x2b4), pal);
    uint8_t *d = ldp(sys, 0x18e0u + 4u * ld16(m, 0x18));
    if (ldp(d, 0xc) == NULL) stp(d, 0xc, g_eng.op_new(0x400));
    memcpy(ldp(d, 0xc), pal, sizeof pal);
    slot_frames_free_all(ldp(sys, 0xaacu + 4u * ld16(m, 0x18)));
    g_eng.h417c40(sys, ld16(m, 0x18), 0);
    *bank = ld16(m, 0x18);
}

void equip_panel_fill(uint8_t *m, uint8_t *win, wl_room *room, int32_t id, int32_t item_id, int32_t priced)
{
    void *sys = g_eng.sys;
    char buf[512];
    /* control 0 (the title row): every cell's text cleared */
    uint8_t *c0 = win_ctrl_at(win, 0);
    const uint8_t *gi = ldp(c0, 0x174);
    if (gi != NULL && ld32(gi, 0x10) > 0) {
        uint16_t r = 0;
        do {
            for (uint16_t k = 0; (int32_t)k < ld32(ldp(c0, 0x174), 8); k++) {
                uint8_t *g = ldp(ldp((uint8_t *)ldp(c0, 0x17c) + 16u * r, 0xc), 24u * k);
                if (g != NULL) glyph_recs_set(g, D(0x8a9b6cu), 0);
            }
            r++;
        } while ((int32_t)r < ld32(ldp(c0, 0x174), 0x10));
    }
    /* the other grids emptied to fresh rows: 2 / 10 / 10 / 1 */
    static const int nrows[5] = { 0, 2, 10, 10, 1 };
    for (uint16_t i = 1; i < 5; i++) {
        uint8_t *c = win_ctrl_at(win, i);
        st32(ldp(c, 0x174), 0x10, 0);
        for (int n = 0; n < nrows[i]; n++) grid_row_add(win_ctrl_at(win, i), 0, 0);
    }
    if (id == 0) {
        g_eng.h5bf3ee(buf, "%s - %s", "Equip", "Don't Equip");
        grid_cell_text(win_ctrl_at(win, 0), 0, 0, buf);
        return;
    }
    const uint8_t *s = NULL;
    for (uint16_t i = 0;; i++) {
        if (i >= 8) return;
        s = (const uint8_t *)room->slot[i];
        if (ld32(s, 0x9f0) == id) break;
    }
    if (!pslot_ok((const wl_pslot *)s, 1, 1) || s == NULL) return;
    const uint8_t *item = item_def(item_id);
    if (item == NULL) return;
    const uint8_t *st = s + 0xa10;   /* the member's stat block */
    g_eng.h5bf3ee(buf, "%s - %s", "Equip", (const char *)st);
    grid_cell_text(win_ctrl_at(win, 0), 0, 0, buf);
    /* control 1: what the member wears in the item's equip slot  >>  the item (+ its price) */
    uint8_t *c1 = win_ctrl_at(win, 1);
    const uint8_t *worn = NULL;
    for (uint16_t k = 0, n = ld16(st, 0x120); k < n; k++) {
        const uint8_t *d = item_def(ld32(st, 0x100 + 4u * k));
        if (d != NULL && ld32(d, 0x78) == ld32(item, 0x78)) {
            worn = d;
            break;
        }
    }
    cell_icon(c1, 0, ld32(sys, 0xb74), 0);
    if (worn != NULL) {
        grid_cell_text(c1, 0, 2, (const char *)worn + 4);
        cell_icon(c1, 1, ld32(sys, 0xb74), ld16(worn, 0x54));
    } else {
        grid_cell_text(c1, 0, 2, "--------");
    }
    grid_cell_text(c1, 0, 3, ">>");
    grid_cell_text(c1, 0, 6, (const char *)item + 4);
    cell_icon(c1, 4, ld32(sys, 0xb74), 0);
    cell_icon(c1, 5, ld32(sys, 0xb74), ld16(item, 0x54));
    if (priced != 0) {
        int32_t kind = ld32(ldp(m, 0xc), 0x4064);
        int32_t v = money_value(kind, ld32(item, 0x60), 1);
        g_eng.h5bf3ee(buf, kind == 10 ? "%dGP" : "%dCP", v);
        grid_cell_text(c1, 1, 6, buf);
        if (ld32(item, 0x60) <= ld32(room, 0x4014)) cell_color_set(c1, 1, 6, 0, 0, 0);
        else cell_color_set(c1, 1, 6, 1, 0xff, 0xff);
    }
    /* control 2 / 3: the stats now >> with the item; control 4: the sprite now >> recoloured */
    uint8_t *c2 = win_ctrl_at(win, 2);
    if (!menu_item_fits(m, id, item)) {
        cell_color_set(c2, 0, 0, 1, 0xff, 0xff);
        grid_cell_text(c2, 0, 0, "Cannot Equip");
        return;
    }
    uint8_t nx[0x4cc] = { 0 };   /* (retail: a stack block; 0x4066d0 sets +0x74..+0x88 for an active member) */
    stats_recompute(room, id, nx, item);
    if (ld32(s, 0x9f0) != 100000104) {
        stat_row_set(c2, 0, "Attack", ld32(st, 0x64) + ld32(st, 0x8c), ld32(st, 0x74), ld32(nx, 0x74), 0x24);
        stat_row_set(c2, 1, "Defense", ld32(st, 0x68) + ld32(st, 0x90), ld32(st, 0x78), ld32(nx, 0x78), 0x25);
        stat_row_set(c2, 2, "Spirit", ld32(st, 0x6c) + ld32(st, 0x94), ld32(st, 0x7c), ld32(nx, 0x7c), 0x26);
        stat_row_set(c2, 3, "Resist", ld32(st, 0x70) + ld32(st, 0x98), ld32(st, 0x80), ld32(nx, 0x80), 0x27);
        uint8_t *c3 = win_ctrl_at(win, 3);
        stat_row_set(c3, 0, "Max HP", ld32(st, 0x58) + ld32(st, 0x9c), ld32(st, 0x84), ld32(nx, 0x84), 0x21);
        stat_row_set(c3, 1, "Max MP", ld32(st, 0x60) + ld32(st, 0xa0), ld32(st, 0x88), ld32(nx, 0x88), 0x22);
    }
    uint8_t *c4 = win_ctrl_at(win, 4);
    uint16_t bank = 0, frame = 0;   /* (retail: stack words; a member without a character leaves them) */
    const uint8_t *clip = NULL;
    member_sprite(m, s, &bank, &frame, &clip, 0, NULL);
    cell_icon_set(c4, 0, 1, ld32(sys, 0xaacu + 4u * bank), (uint16_t)(frame + ld16(clip, 0)),
                  (const uint16_t *)(clip + 2), ld16(clip, 0x42), ld16(clip, 0x44));
    grid_cell_text(c4, 0, 2, ">>");
    member_sprite(m, s, &bank, &frame, &clip, 0, item);
    cell_icon_set(c4, 0, 3, ld32(sys, 0xaacu + 4u * bank), (uint16_t)(frame + ld16(clip, 0)),
                  (const uint16_t *)(clip + 2), ld16(clip, 0x42), ld16(clip, 0x44));
}

int32_t shop_who(uint8_t *m, const uint8_t *item, uint16_t k, int32_t priced)
{
    void *sys = g_eng.sys;
    int32_t res = 0;
    /* the equip preview: title, the slot row (2 x 7), the stats (10 x 5, twice), the sprites (1 x 4) */
    uint8_t *win = sys_win_open(sys, ldp(m, 0), 168, 64, 464, 272, 8, D(0x8a9b6cu), 1);
    uint8_t *g = win_grid_new(win, 24, 48, 2, 7, 2, 1);
    sys_cursor_style(sys, g, 0);
    grid_col_place(g, 0, 0, -8, 0, 0);
    grid_col_place(g, 1, 0, -8, 0, 0);
    grid_col_place(g, 2, 40, 0, 0, 0);
    grid_col_place(g, 3, 176, 0, 0, 0);
    grid_col_place(g, 4, 208, -8, 0, 0);
    grid_col_place(g, 5, 208, -8, 0, 0);
    grid_col_place(g, 6, 248, 0, 0, 0);
    for (int i = 0; i < 2; i++) {
        g = win_grid_new(win, i == 0 ? 32 : 240, 112, 10, 5, 10, 1);
        sys_cursor_style(sys, g, 0);
        grid_col_place(g, 0, 0, -6, 0, 0);
        grid_col_place(g, 1, 32, 0, 0, 0);
        grid_col_place(g, 2, 76, 0, 32, 1);
        grid_col_place(g, 3, 116, 0, 8, 0);
        grid_col_place(g, 4, 132, 0, 32, 1);
    }
    g = win_grid_new(win, 240, 160, 1, 4, 1, 1);
    sys_cursor_style(sys, g, 0);
    grid_col_place(g, 1, 0, 0, 40, 0);
    grid_col_place(g, 2, 64, 40, 8, 0);
    grid_col_place(g, 3, 64, 0, 40, 0);
    /* "On who?": the members + "Don't Equip" */
    wl_room *room = ldp(m, 0xc);
    uint16_t n = (uint16_t)(room_party_count(room, 1, 0) + 1);
    uint8_t *who = sys_win_open(sys, ldp(m, 0), 8, 64, 160, 28 * n + 64, 2, "On who?", 0);
    uint8_t *c = win_grid_new(who, 40, 56, n, 1, n, 0);
    grid_col_place(c, 0, 0, 0, 0, 0);
    sys_cursor_style(sys, c, 1);
    member_rows_add(room, c, 1);
    grid_cell_text(c, grid_row_add(c, 2, 0), 0, "Don't Equip");
    equip_panel_fill(m, win, ldp(m, 0xc), cur_id(c), ld32(item, 0), priced);
    if (ldp(m, 0x24) != NULL) ctrl_text_set(ldp(m, 0x24), "Which character will equip this?");
    for (;;) {
        int32_t r = menu_run(m, c, NULL, 0, 0, 0, 0, 0, 0, 0);
        int32_t id = cur_id(c);
        if (r == 11) break;
        if (r == 13) {
            equip_panel_fill(m, win, ldp(m, 0xc), id, ld32(item, 0), priced);
            continue;
        }
        if (r != 12) { res = r; break; }
        if (id != 0) {
            r = g_eng.h418ec0(m, (intptr_t)pslot_by_id(ldp(m, 0xc), id, 0));
            if (r == 7) continue;
            if (r != 0) { res = r; break; }
            const wl_pslot *s = pslot_by_id(ldp(m, 0xc), id, 1);
            if (s == NULL || !item_fits(s, item)) {
                sfx_play(sys, 6, 0, 0);
                continue;
            }
        }
        sfx_play(sys, 5, 0, 0);
        if (priced == 0) {
            room_item_equip(ldp(m, 0xc), k, id, 1);
            res = 12;
            break;
        }
        uint32_t idx = 0;   /* (retail: a stack dword, 0x415e20 writes its low half) */
        r = g_eng.h415ae0(m, (intptr_t)item, (intptr_t)&idx, 1);
        if (r == 0) continue;
        if (r != 12) { res = r; break; }
        if (id != 0) room_item_equip(ldp(m, 0xc), (uint16_t)idx, id, 1);
        res = 12;
        break;
    }
    st32(win, 0x50, 0);
    st32(who, 0x50, 0);
    menu_sprites_free(m);
    return res;
}

int32_t shop_buy(uint8_t *m, const int32_t *list)
{
    void *sys = g_eng.sys;
    char buf[512], have[512];
    g_eng.h5bf3ee(buf, "%s", "Buy this item");
    uint8_t *win = sys_win_open(sys, ldp(m, 0), 8, 24, 440, 320, 2, buf, 1);
    uint8_t *c = win_grid_new(win, 40, 48, 1000, 5, 8, 2);
    st32(c, 0x1ac, 32);
    grid_col_place(c, 0, 0, -8, 0, 0);
    grid_col_place(c, 1, 0, -8, 0, 0);
    grid_col_place(c, 2, 40, 0, 160, 0);
    grid_col_place(c, 3, 200, 0, 64, 1);
    grid_col_place(c, 4, 264, 0, 112, 1);
    sys_cursor_style(sys, c, 1);
    sys_page_arrows(sys, win, c);
    /* the stock: icon, item icon, name, price */
    for (uint16_t i = 0; list[i] != 0; i++) {
        const uint8_t *it = item_def(list[i]);
        if (it == NULL) continue;
        int32_t row = grid_row_add(c, 0, ld32(it, 0));
        cell_icon_set(c, row, 0, ld32(sys, 0xb74), 0, NULL, 0, 0);
        cell_icon_set(c, row, 1, ld32(sys, 0xb74), ld16(it, 0x54), NULL, 0, 0);
        grid_cell_text(c, row, 2, (const char *)it + 4);
        money_text(buf, ld32(ldp(m, 0xc), 0x4064), ld32(it, 0x60), 1);
        grid_cell_text(c, row, 3, buf);
    }
    menu_have_counts(m, c);
    const uint8_t *it = item_def(cur_id(c));
    int32_t r;
describe:   /* the cursor item's description in the message window */
    if (it != NULL && ldp(m, 0x24) != NULL) ctrl_text_set(ldp(m, 0x24), (const char *)it + 0xac);
run:
    r = menu_run(m, c, NULL, 0, 0, 0, 1, 0, 0, 0);
    it = item_def(cur_id(c));
    if (r == 11) {
        st32(win, 0x50, 0);
        return 0;
    }
    if (r == 13) goto describe;
    if (r != 12) goto out;
    if (it == NULL) goto run;
    if (ld32(it, 0x78) != 0) {   /* gear: who wears it */
        sfx_play(sys, 5, 0, 0);
        r = shop_who(m, it, 0, 1);
    } else {
        r = g_eng.h415ae0(m, (intptr_t)it, 0, 0);
    }
    if (r != 0 && r != 12) goto out;
    for (uint16_t k = 0; (int32_t)k < ld32(ldp(c, 0x174), 0x10); k++) {
        uint16_t cnt = (uint16_t)room_item_count(ldp(m, 0xc), ld32((uint8_t *)ldp(c, 0x17c) + 16u * k, 4), NULL, 0);
        if (cnt != 0) {
            g_eng.h5bf3ee(have, "You have %d", cnt);
            grid_cell_text(c, k, 4, have);
        } else {
            grid_cell_text(c, k, 4, D(0x8a9b6cu));
        }
    }
    goto describe;
out:
    st32(win, 0x50, 0);
    return r;
}

int32_t shop_run(wl_event *ev, const int32_t *list, int32_t a3, int32_t a4)
{
    uint8_t *r = (uint8_t *)ev;
    uint8_t m[0x34] = { 0 };
    int32_t res;
    menu_ctx_clear(m);
    if (a3 != 0) {
        st32(r, 0x588, 1);
    } else if (a4 != 0) {
        st32(r, 0x588, 2);
    } else {
        /* the Buy / Sell choice (the runner's choice records: +0x588 the pick, +0x5a0 + 0x58 x i {text,
         * +0x50, +0x54 value}) */
        st16(r, 0x584, 0);
        st32(r, 0x588, 1);
        st32(r, 0x590, 0);
        st32(r, 0x58c, 0);
        st32(r, 0x594, 0xa0);
        st32(r, 0x598, 0xf0);
        st32(r, 0x59c, 0xf0);
        st32(r, 0x20, 5);
        st32(r, 0x5f4, 1);
        st32(r, 0x5f0, 0);
        strcpy((char *)r + 0x5a0, "Buy");
        uint16_t n = (uint16_t)(ld16(r, 0x584) + 1);
        st16(r, 0x584, n);
        st32(r, 0x5f4 + 0x58u * n, 2);
        st32(r, 0x5f0 + 0x58u * n, 0);
        strcpy((char *)r + 0x5a0 + 0x58u * n, "Sell");
        st16(r, 0x584, (uint16_t)(ld16(r, 0x584) + 1));
        res = script_wait(ev);
        if (res == 6) goto out;
        st32(r, 0x2e4, 1);
        if (ld32(r, 0x588) == 0) {
            res = 11;
            goto out;
        }
    }
    {
        wl_roomloop *rl = ev->f00;
        wl_room *room = ev->room;
        g_eng.h40fe00(g_eng.sys);
        for (uint16_t i = 0; i < rl->nrooms; i++) room_input_flush(&rl->rooms[i]);
        room_views_mark(rl, 0);
        for (int j = 0; j < 32; j++) {
            wl_char *ch = (*g_eng.world)->chars[j];
            if (ld32(ch, 0x1d0) != 0) cmd_clear(&ch->cmd);
        }
        ui_scroll_init(m, (int32_t)(intptr_t)rl, (int32_t)(intptr_t)rl->fe8, (int32_t)(intptr_t)room,
                       (int32_t)(intptr_t)(*g_eng.world)->room->in);
        int32_t v;
        if (ld32(r, 0x588) == 1) {
            res = 0;
            st32(m, 0x14, 0);
            menu_msg_open(m);
            menu_party_open(m);
            v = shop_buy(m, list);
        } else {
            st32(m, 0x30, ld32(r, 0x18));
            res = 0;
            st32(m, 0x14, 0);
            menu_msg_open(m);
            menu_party_open(m);
            v = g_eng.h4192f0(m, 1);
        }
        if (v != 0) res = v;
        else if (ld32(m, 0x14) != 0) res = 12;
        if (ldp(m, 0x28) != NULL) {
            st32(ldp(m, 0x28), 0x50, 0);
            stp(m, 0x28, NULL);
        }
        ui_link_drop(m);
    }
out:
    menu_sprites_free(m);
    return res;
}

/* ---- the item helpers ---- */

int32_t item_kind(const uint8_t *item)
{
    return (ld32(item, 0x2ac) >= 900) + 1;
}

int32_t room_items_of_kind(const wl_room *r, int32_t kind, int32_t unstored)
{
    int32_t n = 0;
    for (uint16_t i = 0, cnt = ld16(r, 0x4010); i < cnt; i++) {
        const uint8_t *rec = (const uint8_t *)r + 0x2010 + 16u * i;
        if (unstored == 1 && rec[0xa] != 0) continue;
        if (item_kind(ldp(rec, 0xc)) == kind) n++;
    }
    return n;
}

int32_t room_items_stored(const wl_room *r)
{
    int32_t n = 0;
    for (uint16_t i = 0, cnt = ld16(r, 0x4010); i < cnt; i++)
        if (((const uint8_t *)r)[0x201a + 16u * i] != 0) n++;
    return n;
}

/* ---- the message windows ---- */

void menu_msg_set(uint8_t *m, const char *text)
{
    /* 0x412590 inlines the steps of 0x40e360: the key tokens (0x412800 each), then parse / records / wrap
     * when the text fits, then the rewind */
    uint8_t *c = ldp(m, 0x24);
    if (c != NULL) ctrl_text_set(c, text);
}

intptr_t menu_party_close(uint8_t *m)
{
    uint8_t *w = ldp(m, 0x28);
    if (w != NULL) {
        st32(w, 0x50, 0);
        stp(m, 0x28, NULL);
    }
    return (intptr_t)w;
}

/* the 9-frame box style {+0 sprite, u16 +4.. frames, +0x18 / +0x1c 32, +0x20 0}: the alternate (sys+0xba8,
 * frames 0, 0..8) or the plain one */
static void box_style(uint8_t *st, int32_t alt)
{
    st32(st, 0x18, 32);
    st32(st, 0x1c, 32);
    if (alt) {
        st32(st, 0, ld32(g_eng.sys, 0xba8));
        st16(st, 4, 0);
    } else {
        st32(st, 0, ld32(g_eng.sys, 0xb88));
        st16(st, 4, 16);
    }
    for (uint16_t k = 0; k < 9; k++) st16(st, 6u + 2u * k, k);
    st32(st, 0x20, 0);
}

static int32_t cfg_line_h(void)
{
    return ld32((const void *)(intptr_t)ld32((const void *)(intptr_t)ld32(g_eng.sys, 0x320), 0), 0x248);
}

uint8_t *msg_window_open(uint8_t *m, const char *text, int32_t x, int32_t y, int32_t center, uint16_t cols,
                         uint16_t rows, int32_t alt, int32_t metrics, int32_t a10)
{
    int32_t w = 7 * cols + 80, h = 28 * rows + 48;
    if (center) {
        x = 320 - w / 2;
        y = 240 - h / 2;
    }
    uint8_t st[0x24];
    box_style(st, alt);
    uint8_t *win = win_mgr_open(ldp(m, 0), x, y, w, h, 1, st);
    uint8_t *c = win_ctrl_take(win);
    if (c != NULL) ctrl_text_init(c, 40, 24, cols, rows);
    sys_msg_keys(g_eng.sys, (int32_t)(intptr_t)win, c);
    static const uint16_t fr[4] = { 0, 1, 2, 3 };
    ctrl_cursor_set(c, ld32(g_eng.sys, 0xb8c), 20, fr, 4, 10, ld32(win, 0x14) - ld32(c, 0xc) - 32,
                    ld32(win, 0x18) - ld32(c, 0x10) - 32);
    uint8_t *t = ldp(c, 0x170);
    if (metrics) {
        int32_t v = cfg_line_h();
        st32(t, 0xc, 1);
        st32(t, 0x10, v);
        st32(t, 0x20, 2 * v);
        st32(t, 0x24, 3 * v);
        st32(t, 0x28, 5 * v);
    } else {
        st32(t, 0xc, 0);
        st32(t, 0x10, 0);
        st32(t, 0x20, 0);
        st32(t, 0x24, 0);
        st32(t, 0x28, 0);
    }
    st32(t, 0x14, a10);
    st32(t, 0x18, 0);
    st32(c, 0x1c, 0);
    ctrl_text_set(c, text);   /* (inline: the key tokens 0x412800, 0x413ed0, the rewind) */
    return win;
}

uint8_t *sys_msg_style(void *sys, uint8_t *st, int32_t alt)
{
    st32(st, 0x18, 32);
    st32(st, 0x1c, 32);
    st16(st, 4, 16);
    st16(st, 6, alt ? 9 : 0);
    st16(st, 8, alt ? 10 : 1);
    st16(st, 0xa, alt ? 11 : 2);
    for (uint16_t k = 3; k <= 8; k++) st16(st, 6u + 2u * k, k);
    st32(st, 0x20, 0);
    st32(st, 0, ld32(sys, 0xb88));
    return st;
}

int32_t msg_pages(uint8_t *m, const char *text, const char *pages, uint16_t n, int32_t x, int32_t y, int32_t center,
                  uint16_t cols, uint16_t rows, int32_t alt, int32_t metrics)
{
    char one[512];
    if (pages != NULL) {
        if (n == 0) return 0;
    } else {
        n = 1;
        strcpy(one, text);
        pages = one;
    }
    int32_t w = 7 * cols + 80, h = 28 * rows + 48;
    if (center) {
        x = 320 - w / 2;
        y = 240 - h / 2;
    }
    uint8_t st[0x24];
    if (alt) box_style(st, 1);
    else sys_msg_style(g_eng.sys, st, 0);
    uint8_t *win = win_mgr_open(ldp(m, 0), x, y, w, h, 1, st);
    uint8_t *c = win_text_add(win, 40, 24, cols, rows);
    sys_msg_keys(g_eng.sys, (int32_t)(intptr_t)win, c);
    sys_box_cursor(g_eng.sys, win, c, 0, 0);
    if (metrics) text_metrics_set(c, 1, cfg_line_h(), 1, 0);
    else text_metrics_set(c, 0, 0, 1, 0);
    ctrl_text_set(c, pages);
    int32_t r, res = 0;
    uint16_t k = 0;
    do {
        for (;;) {
            r = menu_run(m, NULL, win, 1, 0, 0, 0, 0, 0, 0);
            if (r != 12) break;
            if (++k >= n) goto out;
            /* the next page into the window's last control (inline: 0x412800 + 0x40f040 + 0x4031c0 + 0x40e5e0) */
            ctrl_text_set(win_ctrl(win, (uint16_t)(ld16(win, 0x4e) - 1)), pages + 512u * k);
        }
    } while (r == 13);
    res = r;
out:
    st32(win, 0x50, 0);
    return res;
}

/* ---- the item list (the storage / sell / item screens) ---- */

/* the grid info's page top follows the cursor */
static void page_follow(uint8_t *c)
{
    uint8_t *info = ldp(c, 0x174);
    int32_t page = ld32(info, 0xc), cur = ld32(info, 0x14), top = 0;
    while (cur < top || cur >= top + page) top += page;
    if (top != ld32(info, 0x18)) st32(info, 0x18, top);
}
/* the cursor onto the row {0, idx}; 0 = none */
static int cursor_to_item(uint8_t *c, int32_t idx)
{
    uint8_t *info = ldp(c, 0x174);
    int32_t n = ld32(info, 0x10);
    if (n <= 0) return 0;
    const uint8_t *rows = ldp(c, 0x17c);
    for (uint16_t i = 0; (int32_t)i < n; i++) {
        if (ld32(rows, 16u * i) != 0 || ld32(rows, 16u * i + 4) != idx) continue;
        st32(info, 0x14, i);
        page_follow(c);
        return 1;
    }
    return 0;
}
static uint8_t *cell_icon_rec(uint8_t *c, int32_t row, int32_t col)
{
    uint8_t *cells = ldp((uint8_t *)ldp(c, 0x17c) + 16 * row, 0xc);
    if (ldp(cells, 24u * (uint32_t)col + 4) == NULL) stp(cells, 24u * (uint32_t)col + 4, g_eng.op_new(0x54));
    return ldp(cells, 24u * (uint32_t)col + 4);
}
/* a read retail makes through a NULL base (an access violation there too) */
static uint16_t null_read16(uintptr_t va)
{
    __asm__("" : "+r"(va));
    return *(volatile const uint16_t *)va;
}
/* the member slot whose +0x9f0 is `owner` (of the 8), or NULL */
static wl_pslot *slot_of_owner(const wl_room *r, int32_t owner)
{
    for (uint16_t k = 0; k < 8; k++) {
        wl_pslot *s = ldp(r, 0x4030u + 4u * k);
        if (ld32(s, 0x9f0) == owner) return s;
    }
    return NULL;
}

void item_list_fill(uint8_t *m, wl_room *room, int32_t at_item, int32_t at_row, uint8_t *c, int32_t kind, int32_t sort)
{
    uint8_t *r = (uint8_t *)room;
    char buf[512];
    uint8_t done[512];
    st32(ldp(c, 0x174), 0x10, 0);
    int32_t first = -1;
    /* which records stay out: the other side of the box (mode 2 = the stored ones), the other kind (0: no key
     * items), a member's own item when that member is not in the party */
    for (uint16_t i = 0; i < ld16(r, 0x4010); i++) {
        const uint8_t *def = ldp(r, 0x201cu + 16u * i);
        done[i] = 0;
        if (r[0x201au + 16u * i] == 1 ? ld32(m, 0x2c) != 2 : ld32(m, 0x2c) == 2) done[i] = 1;
        if (kind != 0 ? kind != item_kind(def) : ld32(def, 0x2ac) >= 900) done[i] = 1;
        if (ld32(def, 0x2b8) != 0 && pslot_by_id(room, ld32(def, 0x2b8), 0) == NULL) done[i] = 1;
    }
    for (;;) {
        /* the next row: the first left, or (sorted) the worn ones first by the wearer's order, then by
         * category +0x78 (none last), then by id */
        int32_t best = -1, b_owner = 0;
        uint16_t b_eq = 0, b_ord = 0, b_cat = 0;
        uint32_t b_id = 0;
        uint16_t cnt = ld16(r, 0x4010);
        if (cnt == 0) break;
        for (uint16_t i = 0; i < cnt; i++) {
            if (done[i]) continue;
            int32_t owner = ld32(r, 0x2014u + 16u * i);
            const uint8_t *def = ldp(r, 0x201cu + 16u * i);
            uint16_t eq = 0, ord = 0;
            if (owner != 0) {
                eq = 1;
                const uint8_t *s = (const uint8_t *)slot_of_owner(room, owner);
                if (s != NULL && ld32(s, 0x9c4) != 1 && ld32(s, 0x9c4) != 2) s = NULL;
                /* retail reads +0xa0c of NULL here (an access violation): a worn item's wearer is always a
                 * shown party member */
                ord = s != NULL ? ld16(s, 0xa0c) : null_read16(0xa0c);
            }
            uint16_t cat = ld32(def, 0x78) != 0 ? ld16(def, 0x78) : 0xffff;
            uint32_t id = (uint32_t)ld32(def, 0);
            if (best == -1 ||
                (sort != 0 &&
                 (eq > b_eq ||
                  (eq == b_eq && (b_ord > ord || (b_ord == ord && (b_cat > cat || (b_cat == cat && b_id > id)))))))) {
                best = i;
                b_eq = eq;
                b_ord = ord;
                b_cat = cat;
                b_id = id;
                b_owner = owner;
            }
        }
        if (best == -1) break;
        if (first == -1 && b_owner == 0) first = best;
        uint16_t bi = (uint16_t)best;
        done[bi] = 1;
        /* the row {0, record, live} */
        uint8_t *info = ldp(c, 0x174);
        int32_t row = ld32(info, 0x10);
        if (row >= ld32(info, 4)) {
            row = -1;
        } else {
            st32(info, 0x10, row + 1);
            st32(ldp(c, 0x17c), 16u * (uint32_t)row, 0);
            st32(ldp(c, 0x17c), 16u * (uint32_t)row + 4, bi);
            st32(ldp(c, 0x17c), 16u * (uint32_t)row + 8, 1);
            grid_row_reset(c, row);
        }
        const uint8_t *def = bi < ld16(r, 0x4010) ? ldp(r, 0x201cu + 16u * bi) : NULL;
        /* column 0: the icon frame; column 1: the item's icon; 2 the name */
        int32_t spr = ld32(g_eng.sys, 0xb74);
        info = ldp(c, 0x174);
        if (ld32(info, 8) > 0 && ld32(info, 0x10) > row) icon_set(cell_icon_rec(c, row, 0), spr, 0, NULL, 0, 0);
        spr = ld32(g_eng.sys, 0xb74);
        uint16_t fr = ld16(def, 0x54);
        info = ldp(c, 0x174);
        if (ld32(info, 8) > 1 && ld32(info, 0x10) > row) icon_set(cell_icon_rec(c, row, 1), spr, fr, NULL, 0, 0);
        grid_cell_text(c, row, 2, (const char *)def + 4);
        st32(ldp(c, 0x17c), 16u * (uint32_t)row + 8, 1);
        /* column 5: the sell price (mode 1) or the count */
        if (ld32(m, 0x2c) == 1) {
            if (ld32(def, 0x2ac) >= 500) {
                st32(ldp(c, 0x17c), 16u * (uint32_t)row + 8, 0);
                grid_cell_text(c, row, 5, S(0x854adcu));   /* "Cannot sell" */
            } else {
                const uint8_t *rm = ldp(m, 0xc);
                const uint8_t *d2 = bi < ld16(rm, 0x4010) ? ldp(rm, 0x201cu + 16u * bi) : NULL;
                int32_t pct = ld32(m, 0x30) > 0 ? ld32(m, 0x30) : 500, v;
                uint16_t unit = ld16(d2, 0x64);
                int32_t each = (int32_t)((uint32_t)ld32(d2, 0x60) * (uint32_t)pct) / 1000;
                if (unit > 0) {
                    v = (int32_t)((uint32_t)ld16(rm, 0x2018u + 16u * bi) * (uint32_t)each) / unit;
                    if (v < 1) v = 1;
                } else {
                    v = each < 1 ? 1 : each;
                }
                if (ld32(rm, 0x4064) == 10) g_eng.h5bf3ee(buf, S(0x854a34u), (v + 99) / 100);   /* "%dGP" */
                else g_eng.h5bf3ee(buf, S(0x854a20u), v);                                        /* "%dCP" */
                grid_cell_text(c, row, 5, buf);
            }
        } else if (ld16(r, 0x2018u + 16u * bi) != 0) {
            g_eng.h5bf3ee(buf, S(0x854ae8u), ld16(r, 0x2018u + 16u * bi));   /* "x %d" */
            grid_cell_text(c, row, 5, buf);
        }
        /* columns 3 / 4: "E:" + the wearer's name */
        wl_pslot *s = slot_of_owner(room, ld32(r, 0x2014u + 16u * bi));
        if (s == NULL || !pslot_ok(s, 1, 1)) continue;
        grid_cell_text(c, row, 3, S(0x854ad8u));   /* "E:" */
        s = slot_of_owner(room, ld32(r, 0x2014u + 16u * bi));
        if (s != NULL && !pslot_ok(s, 1, 1)) s = NULL;
        grid_cell_text(c, row, 4, (const char *)((uintptr_t)s + 0xa10));
    }
    /* the cursor: onto row `at_row`, else the record `at_item`, else the first unworn row */
    if (at_row != -1) {
        if (at_row < 0) return;
        uint8_t *info = ldp(c, 0x174);
        int32_t last = ld32(info, 0x10) - 1;
        int32_t v = last < at_row ? last : at_row;
        st32(info, 0x14, v < 0 ? 0 : v);
        page_follow(c);
    } else if (at_item != -1) {
        int32_t last = (int32_t)ld16(r, 0x4010) - 1;
        cursor_to_item(c, last < at_item ? last : at_item);
    } else if (first != -1) {
        cursor_to_item(c, first);
    }
}

/* ---- the storage box (a talk record's 0x411610 on the event runner) ---- */

/* the box side's frame: `deposit` 1 = "Deposit Items" (the unstored items of `kind`), 0 = "Withdraw Items" */
static int32_t storage_side(uint8_t *m, int32_t deposit, int32_t kind)
{
    int32_t res = 0, shown = 0, r;
    uint8_t *win = sys_win_open(g_eng.sys, ldp(m, 0), 8, 24, 408, 320, 2, S(deposit ? 0x8549ecu : 0x8549dcu), 1);
    uint8_t *c = win_grid_new(win, 40, 48, 512, 6, 8, 2);
    st32(c, 0x1ac, 32);
    grid_col_place(c, 0, 0, -8, 0, 0);
    grid_col_place(c, 1, 0, -8, 0, 0);
    grid_col_place(c, 2, 40, 0, 160, 0);
    grid_col_place(c, 3, 200, 0, 64, 0);
    grid_col_place(c, 4, 216, 0, 64, 0);
    st32(m, 0x2c, deposit ? 0 : 2);
    grid_col_place(c, 5, 280, 0, 56, 0);
    sys_cursor_style(g_eng.sys, c, 1);
    sys_page_arrows(g_eng.sys, win, c);
    item_list_fill(m, ldp(m, 0xc), -1, -1, c, kind, ld32(m, 0x10));
    if (ldp(m, 0x24) != NULL)   /* "Please select an item to store / withdraw. Press @@Z to re-arrange your items." */
        ctrl_text_set(ldp(m, 0x24), S(deposit ? 0x854a94u : 0x854b34u));
    while (ld32(ldp(c, 0x174), 0x10) != 0) {
        if (shown) {   /* the cursor's item description */
            int32_t idx = ld32(ldp(c, 0x17c), 16u * (uint32_t)ld32(ldp(c, 0x174), 0x14) + 4);
            const uint8_t *rm = ldp(m, 0xc);
            if ((uint16_t)idx < ld16(rm, 0x4010)) {
                const uint8_t *def = ldp(rm, 0x201cu + 16u * (uint16_t)idx);
                if (def != NULL && ldp(m, 0x24) != NULL) ctrl_text_set(ldp(m, 0x24), (const char *)def + 0xac);
            }
        }
        r = menu_run(m, c, NULL, 1, 0, 1, 1, 0, 1, 0);
        int32_t cur = ld32(ldp(c, 0x174), 0x14);
        int32_t idx = ld32(ldp(c, 0x17c), 16u * (uint32_t)cur + 4);
        wl_room *room = ldp(m, 0xc);
        uint8_t *rec = (uint8_t *)room + 0x2010 + 16u * (uint16_t)idx;
        const uint8_t *def = (uint16_t)idx < ld16(room, 0x4010) ? ldp(rec, 0xc) : NULL;
        switch (r) {
        case 11:
            goto out;
        case 12:
            if (def == NULL) continue;
            if (deposit ? room_items_stored(room) >= 128
                        : item_kind(ldp(rec, 0xc)) == 1 && room_items_of_kind(room, 1, 1) >= 48) {
                /* "You cannot store anything else." / "You cannot carry any more items." */
                r = msg_pages(m, S(deposit ? 0x854a74u : 0x854b10u), NULL, 0, 0, 0, 1, 40, 3, 0, 0);
                if (r != 0) {
                    res = r;
                    goto out;
                }
                break;
            }
            if (deposit) {
                rec[0xa] = 1;
                st32(rec, 4, 0);   /* (deposited gear comes off) */
            } else {
                rec[0xa] = 0;
            }
            room_stats_refresh(room, 1);
            menu_party_fill(m);
            item_list_fill(m, ldp(m, 0xc), -1, (uint16_t)cur, c, kind, ld32(m, 0x10));
            shown = 1;
            break;
        case 13:
            shown = 1;
            break;
        case 20:   /* @@Z: sort on / off */
            st32(m, 0x10, ld32(m, 0x10) == 0);
            sfx_play(g_eng.sys, 9, 0, 0);
            item_list_fill(m, ldp(m, 0xc), (uint16_t)idx, -1, c, kind, ld32(m, 0x10));
            continue;
        default:
            res = r;
            goto out;
        }
    }
    {
        /* "You have nothing to store." / "You don't have anything stored." (inline 0x4140c0, one page) */
        char text[512];
        strcpy(text, S(deposit ? 0x854a58u : 0x854af0u));
        uint8_t *mw = msg_window_open(m, text, 0, 0, 1, 40, 3, 0, 0, 1);
        for (;;) {
            r = menu_run(m, NULL, mw, 1, 0, 0, 0, 0, 0, 0);
            if (r == 12) {
                r = 0;
                break;
            }
            if (r != 13) break;
        }
        st32(mw, 0x50, 0);
        if (r != 0) res = r;
    }
out:
    st32(win, 0x50, 0);
    return res;
}

int32_t storage_deposit(uint8_t *m, int32_t kind) { return storage_side(m, 1, kind); }
int32_t storage_withdraw(uint8_t *m) { return storage_side(m, 0, 0); }

int32_t storage_run(wl_event *ev)
{
    uint8_t m[0x34] = { 0 };   /* (retail clears +0..+0x10, +0x1c..+0x2c, +0x18, +0x30; the rest is unread) */
    wl_roomloop *rl = ev->f00;
    wl_room *room = ev->room;
    int32_t res = 0;
    g_eng.h40fe00(g_eng.sys);
    for (uint16_t i = 0; i < rl->nrooms; i++) room_input_flush(&rl->rooms[i]);
    room_views_mark(rl, 0);
    for (int j = 0; j < 32; j++) {
        wl_char *ch = (*g_eng.world)->chars[j];
        if (ld32(ch, 0x1d0) != 0) cmd_clear(&ch->cmd);
    }
    menu_ctx_clear(m);
    stp(m, 8, (*g_eng.world)->room->in);
    stp(m, 0, rl->fe8);
    stp(m, 4, rl);
    stp(m, 0xc, room);
    st32(m, 0x10, 1);
    menu_msg_open(m);
    menu_party_open(m);
    uint8_t *win, *c;
    sys_list_open(g_eng.sys, ldp(m, 0), 8, 8, 224, 0, 2, 3, 1, &win, &c, S(0x8549fcu), 1);   /* "Item Storage" */
    grid_row_text_add(c, 0, 21, S(0x8549ecu));   /* "Deposit Items" */
    grid_row_text_add(c, 0, 22, S(0x8549dcu));   /* "Withdraw Items" */
    grid_row_text_add(c, 0, 25, S(0x8549d0u));   /* "Close Box" */
    for (;;) {
        char buf[512];
        g_eng.h5bf3ee(buf, S(0x85499cu), 128);   /* "You can store up to %d items in your storage box." */
        menu_msg_set(m, buf);
        int32_t r = menu_run(m, c, NULL, 1, 0, 0, 1, 0, 0, 0);
        if (r == 6) {
            res = 6;
            break;
        }
        if (r == 13) continue;
        if (r != 12) break;
        int32_t pick = ld32(ldp(c, 0x17c), 16u * (uint32_t)ld32(ldp(c, 0x174), 0x14) + 4);
        int32_t v;
        if (pick == 21) v = storage_deposit(m, 1);
        else if (pick == 22) v = storage_withdraw(m);
        else break;
        if (v == 0) continue;
        if (v == 6) res = 6;
        break;
    }
    st32(win, 0x50, 0);
    menu_party_close(m);
    ui_link_drop(m);
    uint16_t idx = ld16(m, 0x18);
    if (idx != 0) {
        slot_frames_free_all(ldp(g_eng.sys, 0xaacu + 4u * idx));
        st32(ldp(g_eng.sys, 0x18e0u + 4u * idx), 0x10, 0);
    }
    return res;
}

/* ---- the book reader (a chapter script's 0x40dbc0: a bookshelf's book) ---- */

int32_t book_read(wl_event *ev, const uint8_t *book)
{
    uint8_t m[0x34] = { 0 };   /* (retail clears +0..+0x10, +0x1c..+0x2c, +0x18, +0x30; the rest is unread) */
    wl_roomloop *rl = ev->f00;
    wl_room *room = ev->room;
    int32_t res = 0;
    g_eng.h40fe00(g_eng.sys);
    for (uint16_t i = 0; i < rl->nrooms; i++) room_input_flush(&rl->rooms[i]);
    room_views_mark(rl, 0);
    for (int j = 0; j < 32; j++) {
        wl_char *ch = (*g_eng.world)->chars[j];
        if (ld32(ch, 0x1d0) != 0) cmd_clear(&ch->cmd);
    }
    menu_ctx_clear(m);
    stp(m, 8, (*g_eng.world)->room->in);
    stp(m, 0xc, room);
    stp(m, 0, rl->fe8);
    stp(m, 4, rl);
    st32(m, 0x10, 1);
    char title[512];
    g_eng.h5bf3ee(title, S(0x854854u), (const char *)book);   /* "'%s'" */
    uint8_t *win = sys_win_open(g_eng.sys, ldp(m, 0), 120, 8, 400, 464, 4, title, 0);
    uint8_t *c = win_text_add(win, 40, 48, 44, 14);
    sys_msg_keys(g_eng.sys, (int32_t)(intptr_t)win, c);
    sys_box_cursor(g_eng.sys, win, c, -8, -8);
    st32(ldp(c, 0x170), 0x1c, 12);
    text_metrics_set(c, 0, 0, 1, 0);
    st32(c, 0x1a4, ld32(g_eng.sys, 0x2728));
    sys_page_arrows(g_eng.sys, win, c);
    /* the pages: 2052 B each from +0x28 (the text), the first with its end word +0x800 set counts them */
    uint16_t n = 0;
    if (ld32(book, 0x828) == 0)
        do n++; while (ld32(book, 0x828u + 2052u * n) == 0);
    uint16_t page = 0;
    int show = 1;
    for (;;) {
        if (show) {
            show = 0;
            sfx_play(g_eng.sys, 0, 0, 0);
            ctrl_text_set(c, (const char *)book + 2052u * page + 0x28);
            st16(c, 0x162, n);
            st16(c, 0x160, (uint16_t)(page + 1));
        }
        int32_t r = menu_run(m, NULL, win, 0, 1, 0, 1, 0, 0, 0);
        if (r == 12) {   /* on: the next page, or closed after the last */
            page++;
            show = 1;
            if (page < n) continue;
            sfx_play(g_eng.sys, 5, 0, 0);
            res = 0;
            break;
        }
        if (r == 13) continue;
        if (r == 16) {   /* back a page */
            if (page != 0) {
                page--;
                show = 1;
            }
            continue;
        }
        if (r == 17) {   /* on a page */
            if ((int32_t)page < (int32_t)n - 1) {
                page++;
                show = 1;
            }
            continue;
        }
        res = r;
        break;
    }
    st32(win, 0x50, 0);
    if (ld16(m, 0x18) != 0) sprite_set_free(g_eng.sys, ld16(m, 0x18));
    return res;
}
