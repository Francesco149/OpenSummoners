/* screen_fx.c — the per-frame screen effects.  See screen_fx.h.  Transcribed from the asm. */
#include "screen_fx.h"

#include <stddef.h>
#include <string.h>

#include "asset_register.h"
#include "engine_calls.h"
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

/* cell k (when it exists) starts fading, its clock staggered */
static void cell_start(uint8_t *fx, int32_t k, uint16_t clock)
{
    if (k < 0 || k >= ld16(fx, 8)) return;
    uint8_t *c = (uint8_t *)ldp(fx, 0) + 12 * k;
    if (ld16(c, 0) != 0) return;
    st16(c, 0, 1);
    st16(c, 2, clock);
}

void fx_rows_center_out(uint8_t *fx, uint16_t step, uint16_t rows)
{
    for (uint16_t i = 0; i < rows; i++) {
        uint16_t clock = (uint16_t)((rows - i - 1) * (step / rows));
        for (uint16_t j = 0; j < ld16(fx, 4); j++) {
            int32_t mid = ld16(fx, 6) >> 1, r = ld16(fx, 0x14);
            cell_start(fx, ld16(fx, 4) * (mid - r) + j, clock);
            cell_start(fx, ld16(fx, 4) * (r + mid) + j, clock);
        }
        if (ld16(fx, 0x14) < (uint16_t)(ld16(fx, 6) >> 1)) st16(fx, 0x14, (uint16_t)(ld16(fx, 0x14) + 1));
    }
}

void fx_rows_edges_in(uint8_t *fx, uint16_t step, uint16_t rows)
{
    for (uint16_t i = 0; i < rows; i++) {
        uint16_t clock = (uint16_t)((rows - i - 1) * (step / rows));
        for (uint16_t j = 0; j < ld16(fx, 4); j++) {
            int32_t r = ld16(fx, 0x14);
            cell_start(fx, j + r * ld16(fx, 4), clock);
            cell_start(fx, ld16(fx, 4) * (ld16(fx, 6) - r) + j, clock);
        }
        if (ld16(fx, 0x14) < (uint16_t)(ld16(fx, 6) >> 1)) st16(fx, 0x14, (uint16_t)(ld16(fx, 0x14) + 1));
    }
}

void fx_rows_sweep_down(uint8_t *fx, uint16_t step, uint16_t rows)
{
    for (uint16_t i = 0; i < rows; i++) {
        uint16_t clock = (uint16_t)((rows - i - 1) * (step / rows));
        for (uint16_t j = 0; j < ld16(fx, 4); j++) cell_start(fx, ld16(fx, 4) * ld16(fx, 0x14) + j, clock);
        if (ld16(fx, 0x14) < ld16(fx, 6)) st16(fx, 0x14, (uint16_t)(ld16(fx, 0x14) + 1));
    }
}

void fx_rows_sweep_up(uint8_t *fx, uint16_t step, uint16_t rows)
{
    for (uint16_t i = 0; i < rows; i++) {
        uint16_t clock = (uint16_t)((rows - i - 1) * (step / rows));
        for (uint16_t j = 0; j < ld16(fx, 4); j++)
            cell_start(fx, ld16(fx, 4) * (ld16(fx, 6) - ld16(fx, 0x14) - 1) + j, clock);
        if (ld16(fx, 0x14) < ld16(fx, 6)) st16(fx, 0x14, (uint16_t)(ld16(fx, 0x14) + 1));
    }
}

/* the hold clock (phases 0 and 1): count to `hold`, then the next phase */
static int32_t hold_step(uint8_t *o, int32_t hold)
{
    if ((int32_t)ld16(o, 6) < hold) st16(o, 6, (uint16_t)(ld16(o, 6) + 1));
    else st16(o, 4, (uint16_t)(ld16(o, 4) + 1));
    return 0;
}

/* alpha up by `in` (to 1000); there: the hold clock restarts, phase 1 */
static void fade_in(uint8_t *o, int32_t in)
{
    int32_t a = ld32(o, 8);
    if (a < 1000) {
        a += in;
        st32(o, 8, a > 1000 ? 1000 : a);
    }
    if (ld32(o, 8) >= 1000) {
        st16(o, 6, 0);
        st16(o, 4, (uint16_t)(ld16(o, 4) + 1));
    }
}

int32_t overlay_fade(uint8_t *o, int32_t in, int32_t out, int32_t hold)
{
    switch (ld16(o, 4)) {
    case 0:
        fade_in(o, in);
        return hold_step(o, hold);
    case 1:
        return hold_step(o, hold);
    case 2:
        if (ld32(o, 8) > 0) {
            int32_t a = ld32(o, 8) - out;
            st32(o, 8, a < 0 ? 0 : a);
            return 0;
        }
        st32(o, 0x20, 0);
        return 1;
    default:
        return 0;
    }
}

/* every entry's frames of the system sprite slot at sys+off */
static void slot_free(uint32_t off)
{
    ar_sprite_slot *s = ldp(g_eng.sys, off);
    for (uint16_t e = 0; e < s->entry_count; e++) slot_frames_free(s, e);
}

void overlay_tick(uint8_t *o)
{
    if (ld32(o, 0x20) == 0) return;
    int32_t kind = ld32(o, 0), in, out, hold;
    if (kind == 1) {
        switch (ld16(o, 4)) {
        case 0: {
            ar_sprite_slot *s = ldp(g_eng.sys, 0xbb4);
            if (s->entries->frames != NULL) slot_free(0xbb4);
            st16(o, 4, (uint16_t)(ld16(o, 4) + 1));
        }   /* fall through */
        case 1: {
            int32_t a = ld32(o, 8);
            if (a < 1000) {
                a += 20;
                st32(o, 8, a > 1000 ? 1000 : a);
            }
            if (ld32(o, 8) >= 1000) st16(o, 4, (uint16_t)(ld16(o, 4) + 1));
            return;
        }
        case 2:
            if (ld16(o, 6) >= ld16(o, 0x10)) st16(o, 4, (uint16_t)(ld16(o, 4) + 1));
            else st16(o, 6, (uint16_t)(ld16(o, 6) + 1));
            return;
        case 3:
            if (ld32(o, 8) > 0) {
                int32_t a = ld32(o, 8) - 20;
                st32(o, 8, a < 0 ? 0 : a);
                return;
            }
            st32(o, 0x20, 0);
            slot_free(0xbb4);
            return;
        default:
            return;
        }
    }
    if (kind == 2) { in = 40; out = 40; hold = 200; }
    else if (kind == 3) { in = 20; out = 20; hold = 300; }
    else if (kind == 4) { in = 20; out = 20; hold = 600; }
    else return;
    switch (ld16(o, 4)) {
    case 0:
        fade_in(o, in);
        /* fall through */
    case 1:
        if (ld16(o, 6) < hold) st16(o, 6, (uint16_t)(ld16(o, 6) + 1));
        else st16(o, 4, (uint16_t)(ld16(o, 4) + 1));
        return;
    case 2:
        if (ld32(o, 8) > 0) {
            int32_t a = ld32(o, 8) - out;
            st32(o, 8, a < 0 ? 0 : a);
            return;
        }
        st32(o, 0x20, 0);
        slot_free(0xbb8);
        return;
    default:
        return;
    }
}

/* the world's notice records: 8 x 516 B from world+4 {+0 pending, +4 the text}, world+0x1024 the next */
static uint8_t *notice_rec(uint32_t k) { return (uint8_t *)g_world + 4 + 516u * k; }

void notice_tick(wl_roomloop *rl)
{
    uint8_t *L = (uint8_t *)rl;
    if (ldp(L, 0xec) == NULL) {
        /* the next pending notice (round robin) opens a sliding window */
        uint8_t *rec;
        for (uint16_t n = 0;; ) {
            int32_t k = ld32(g_world, 0x1024);
            if (ld32(notice_rec((uint32_t)k), 0) != 0) break;
            n++;
            st32(g_world, 0x1024, (k + 1) % 8);
            if (n >= 8) return;
        }
        rec = notice_rec((uint32_t)ld32(g_world, 0x1024));
        st32(rec, 0, 0);
        void *sys = g_eng.sys;
        uint8_t st[0x24];
        static const uint16_t fr[10] = { 0, 0, 1, 2, 3, 4, 5, 6, 7, 8 };
        st32(st, 0x18, 32);
        st32(st, 0x1c, 32);
        memcpy(st + 4, fr, sizeof fr);
        st32(st, 0, ld32(sys, 0xbac));
        if (ld32(ldp(ldp(sys, 0x320), 0), 0x94) == 0) {
            st32(st, 0, 0);
            st32(st, 0x20, 0);
        } else {
            st32(st, 0x20, ld32(sys, 0x27d0));
        }
        uint8_t *win = win_mgr_open(ldp(L, 0xe8), 160, 384, 320, 80, 3, st);
        if (win == NULL) {
            st32(rec, 0, 1);   /* (no window free: retried next frame) */
            return;
        }
        win_anim_set(win, 2, 1);   /* slides in from the right, out to the left */
        st32(win, 0x38, ld32(win, 0xc));
        st32(win, 0x20, 640);
        st32(win, 0x24, 384);
        st32(win, 0x28, -480);
        st32(win, 0x2c, 384);
        st32(win, 0x3c, ld32(win, 0x10));
        st32(win, 0x30, 0);
        st32(win, 0x34, 1000);
        st32(win, 0xc, 640);
        st32(win, 0x10, 384);
        st32(win, 0x40, 0);
        st32(win, 0x44, 0);
        uint8_t *c = win_ctrl_take(win);
        if (c != NULL) ctrl_text_init(c, 40, 24, 40, 2);
        char buf[512];
        uint16_t width = ld16(ldp(c, 0x170), 8);
        const char *txt = ctrl_keys_expand(c, (const char *)rec + 4, buf);
        text_layout_set(ldp(ldp(c, 0x170), 0), txt, width);
        uint8_t *t = ldp(c, 0x170);
        st16(t, 4, 0);
        st16(t, 6, 0);
        st32(c, 0x1c, 0);
        st32(t, 0xc, 0);
        st32(t, 0x10, 0);
        st32(t, 0x20, 0);
        st32(t, 0x24, 0);
        st32(t, 0x28, 0);
        st32(t, 0x14, 0);
        st32(t, 0x18, 120);   /* shown 120 frames */
        st32(c, 0x1c, 0);
        st32(c, 0x180, 0xffffff);
        st32(c, 0x184, 0x808080);
        st32(c, 0x188, 0xc0c0c0);
        win_ctrl_to_back(ldp(c, 0), ld16(c, 4));
        sys_msg_keys(sys, (int32_t)(intptr_t)win, c);
        stp(L, 0xec, win);
    }
    uint8_t *win = ldp(L, 0xec);
    if (win == NULL) return;
    for (uint32_t k = 0; k < 8; k++) {   /* another one waiting: this one's hold shortened to 50 */
        if (ld32(notice_rec(k), 0) == 0) continue;
        uint8_t *t = ldp(((uint8_t **)ldp(win, 0x48))[ld16(win, 0x4e) - 1], 0x170);
        if (t != NULL && (uint32_t)ld32(t, 0x18) > 50) st32(t, 0x18, 50);
        break;
    }
    if (win_tick(ldp(L, 0xec)) == 8) {
        st32(ldp(L, 0xec), 0x50, 0);
        stp(L, 0xec, NULL);
    }
}
