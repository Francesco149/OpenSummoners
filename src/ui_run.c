/* ui_run.c — the UI toolkit's runtime.  See ui_run.h.  Transcribed from the asm. */
#include "ui_run.h"

#include <stddef.h>
#include <string.h>

#include "char_ai.h"      /* cmd_clear 0x411530 */
#include "engine_calls.h"
#include "room_loop.h"    /* timed_sounds_tick 0x43e140, room_input_update 0x468a20 */
#include "sound_fx.h"     /* sfx_play 0x411390, vpool_take 0x40b710 */
#include "world_layout.h"
#include "world_tick.h"   /* world_tick 0x46cd70 */

static int32_t  ld32(const void *p, uint32_t off) { int32_t v; memcpy(&v, (const uint8_t *)p + off, 4); return v; }
static uint16_t ld16(const void *p, uint32_t off) { uint16_t v; memcpy(&v, (const uint8_t *)p + off, 2); return v; }
static void st32(void *p, uint32_t off, int32_t v) { memcpy((uint8_t *)p + off, &v, 4); }
static void st16(void *p, uint32_t off, uint16_t v) { memcpy((uint8_t *)p + off, &v, 2); }
static void *ldp(const void *p, uint32_t off) { void *v; memcpy(&v, (const uint8_t *)p + off, sizeof v); return v; }
static void *sysp(uint32_t off) { return ldp(g_eng.sys, off); }

/* a window's last taken control */
static uint8_t *win_last(const uint8_t *win) { return ((uint8_t **)ldp(win, 0x48))[ld16(win, 0x4e) - 1]; }

/* the frame clock of an icon record at `ic` (the cell icons, the arrows at +0xce / +0x12a - 0x46) */
uint8_t *icon_tick(uint8_t *ic)
{
    uint16_t n = ld16(ic, 0x46);
    if (n == 0) return ic;
    st16(ic, 0x4c, (uint16_t)(ld16(ic, 0x4c) + 1));
    if (ld16(ic, 0x4c) < ld16(ic, 0x48)) return ic;
    st16(ic, 0x4a, (uint16_t)(ld16(ic, 0x4a) + 1));
    uint16_t f = ld16(ic, 0x4a);
    st16(ic, 0x4c, 0);
    if (f >= n) st16(ic, 0x4a, 0);
    return ic;
}

int32_t text_key(uint8_t *c, int32_t key)
{
    uint8_t *t = ldp(c, 0x170);
    int32_t mode = ld32(t, 0xc);
    if (mode == 0) {
        if (key < 8 || key > 10) return 0;
        if (ld32(t, 0x18) != 0) {
            st32(t, 0x18, 0);
            return 6;
        }
        if (ld32(t, 0x14) == 0) return 0;
        st32(c, 0x1c, 8);
        return 8;
    }
    if (mode != 1 || key < 8 || key > 10 || ld32(c, 0x1c) == 8) return 0;
    uint16_t n = ld16(ldp(ldp(t, 0), 0xc), 8);
    if (t != NULL && ld16(t, 4) >= n) {
        st32(c, 0x1c, 8);
        return 8;
    }
    st16(t, 4, n);
    st32(c, 0x1c, 6);
    return 6;
}

static int32_t imin(int32_t a, int32_t b) { return a < b ? a : b; }

/* the page holding the cursor (from 0 in steps of the page size) */
static int32_t page_of(const uint8_t *g)
{
    int32_t top = 0, page = ld32(g, 0xc), cur = ld32(g, 0x14);
    while (cur < top || cur >= page + top) top += page;
    return top;
}

int32_t grid_nav(uint8_t *c, int32_t key)
{
    uint8_t *g = ldp(c, 0x174);
    int32_t ret = 0;
    int32_t end = ld32(g, 0x18) + ld32(g, 0xc);   /* the shown page's end, before the move */
    switch (key) {
    case 4: case 6: {   /* held: the first repeat after 300 ms, then every 100 */
        uint32_t now = g_eng.tick();
        uint32_t off = key == 4 ? 0x1c : 0x20;
        g = ldp(c, 0x174);
        uint32_t due = (uint32_t)ld32(g, off);
        if (due == 0) {
            st32(g, off, (int32_t)(now + 300));
        } else if (now >= due) {
            key = key == 4 ? 1 : 0;
            st32(g, off, (int32_t)(now + 100));
        }
        break;
    }
    case 5: st32(g, 0x1c, 0); break;
    case 7: st32(g, 0x20, 0); break;
    default: break;
    }
    switch (key) {
    case 0: {   /* up */
        g = ldp(c, 0x174);
        int32_t n = ld32(g, 0x10);
        if (n <= 1) break;
        int32_t cur = ld32(g, 0x14);
        if (ld32(g, 0) == 0) {
            st32(g, 0x14, cur == ld32(g, 0x18) ? imin(n - 1, end - 1) : cur - 1);
        } else if (ld32(g, 0) == 2) {
            int32_t page = ld32(g, 0xc);
            if (cur > 0 && cur > page * (cur / page)) st32(g, 0x14, cur - 1);
            else st32(g, 0x14, imin(n - 1, page + cur - 1));
        } else {
            st32(g, 0x14, cur - 1);
            g = ldp(c, 0x174);
            if (cur - 1 < 0) st32(g, 0x14, ld32(g, 0x10) + (cur - 1));
        }
        ret = 1;
        break;
    }
    case 1: {   /* down */
        g = ldp(c, 0x174);
        int32_t n = ld32(g, 0x10);
        if (n <= 1) break;
        int32_t cur = ld32(g, 0x14);
        if (ld32(g, 0) == 0) {
            st32(g, 0x14, cur == imin(n - 1, end - 1) ? ld32(g, 0x18) : cur + 1);
        } else if (ld32(g, 0) == 2) {
            int32_t page = ld32(g, 0xc);
            if (cur < n - 1 && cur < page * (cur / page + 1) - 1) st32(g, 0x14, cur + 1);
            else st32(g, 0x14, page * (cur / page));
        } else {
            st32(g, 0x14, cur + 1);
            g = ldp(c, 0x174);
            int32_t v = ld32(g, 0x14), m = ld32(g, 0x10);
            if (v >= m) st32(g, 0x14, v % m);
        }
        ret = 1;
        break;
    }
    case 2: case 3: {   /* a page back / on: the same column of the previous / next page */
        g = ldp(c, 0x174);
        int32_t n = ld32(g, 0x10), page = ld32(g, 0xc);
        if (n <= 1 || page >= n) break;
        int32_t cur = ld32(g, 0x14), last = n - 1;
        if (key == 2) {
            if (cur - page >= 0) {
                st32(g, 0x14, cur - page);
            } else {
                int32_t v = page * (last / page) + cur % page;
                st32(g, 0x14, imin(last, v) < 0 ? 0 : imin(last, v));
            }
        } else {
            if (page + cur < n) st32(g, 0x14, page + cur);
            else if (page + cur < page * (last / page + 1)) st32(g, 0x14, last);
            else st32(g, 0x14, imin(last, cur % page));
        }
        g = ldp(c, 0x174);
        int32_t top = page_of(g);
        if (top == ld32(g, 0x18)) return 0;
        st32(g, 0x18, top);
        g = ldp(c, 0x174);
        if (ld32(g, 0) == 3) st32(g, 0x14, ld32(g, 0x18));
        return 2;
    }
    case 9: ret = 3; st32(c, 0x1c, 3); break;
    case 10: ret = 4; st32(c, 0x1c, 4); break;
    default: return 0;   /* (a held key not yet repeating, a release, 8) */
    }
    g = ldp(c, 0x174);
    if (ld32(g, 0) == 2) {
        int32_t top = page_of(g);
        if (top != ld32(g, 0x18)) {
            st32(g, 0x18, top);
            ret = 2;
        }
    } else if (ld32(g, 0) == 3) {
        if (ld32(g, 0x18) > ld32(g, 0x14)) st32(g, 0x18, ld32(g, 0x14));
        g = ldp(c, 0x174);
        if (end <= ld32(g, 0x14)) st32(g, 0x18, ld32(g, 0x14) - ld32(g, 0xc) + 1);
    }
    return ret;
}

int32_t ctrl_key(uint8_t *c, int32_t key)
{
    const uint8_t *win = ldp(c, 0);
    if (ld32(win, 0x54) != 1000 || ld32(win, 4) == 0) return 0;
    if (ld32(c, 8) == 1) return grid_nav(c, key);
    if (ld32(c, 8) == 2) return text_key(c, key);
    return 0;
}

int32_t ctrl_tick(uint8_t *c)
{
    int32_t r = 0;
    if (ld32(c, 8) == 1) {   /* the shown rows' cell icons */
        uint8_t *g = ldp(c, 0x174);
        for (int32_t row = ld32(g, 0x18); row < ld32(ldp(c, 0x174), 0x10); row++)
            for (int32_t k = 0; k < ld32(ldp(c, 0x174), 8); k++) {
                uint8_t *ic = ldp(ldp((uint8_t *)ldp(c, 0x17c) + 16 * row, 0xc), 24u * (uint32_t)k + 4);
                if (ic != NULL) icon_tick(ic);
            }
        st32(c, 0x1c, 0);
    } else if (ld32(c, 8) == 2) {   /* the typewriter */
        uint8_t *t = ldp(c, 0x170);
        int32_t mode = ld32(t, 0xc);
        if (mode == 1) {
            if (ld32(c, 0x1c) == 8) {
                r = 8;
            } else if (ld16(t, 4) >= ld16(ldp(ldp(t, 0), 0xc), 8)) {   /* all shown: the hold, then done */
                if (ld32(t, 0x18) != 0) {
                    st32(t, 0x18, ld32(t, 0x18) - 1);
                    st32(c, 0x1c, 0);
                } else {
                    r = ld32(t, 0x14) != 0 ? 7 : 8;
                    st32(c, 0x1c, r);
                }
            } else {   /* the next glyph after the delay of the last one's break class */
                uint16_t shown = ld16(t, 4);
                uint32_t delay = 0;
                int pause = 0;
                if (shown != 0) {
                    switch (((const uint8_t *)ldp(ldp(t, 0), 0))[12u * (uint16_t)(shown - 1) + 9]) {
                    case 1: delay = (uint32_t)ld32(t, 0x20); break;
                    case 2: case 5: delay = (uint32_t)ld32(t, 0x24); break;
                    case 3: delay = (uint32_t)ld32(t, 0x28); break;
                    case 4: delay = 0; pause = 1; break;
                    default: delay = (uint32_t)ld32(t, 0x10); break;
                    }
                }
                if (ld16(t, 6) < delay) {
                    st16(t, 6, (uint16_t)(ld16(t, 6) + 1));
                    st32(c, 0x1c, 0);
                } else {
                    r = pause ? 0 : 5;
                    st16(t, 6, 0);
                    uint8_t *t2 = ldp(c, 0x170);
                    st16(t2, 4, (uint16_t)(ld16(t2, 4) + 1));
                    st32(c, 0x1c, r);
                }
            }
        } else if (mode == 0) {
            if (ld32(t, 0x18) != 0) st32(t, 0x18, ld32(t, 0x18) - 1);
            else if (ld32(t, 0x14) != 0 && ld32(c, 0x1c) != 8) st32(c, 0x1c, r = 7);
            else r = 8;
        }
    }
    if (ld32(c, 0x84) != 0) {   /* the page arrows */
        icon_tick(c + 0xce - 0x46);
        icon_tick(c + 0x12a - 0x46);
    }
    if (ld32(c, 0x20) != 0) icon_tick(c + 0x28);   /* the cursor */
    return r;
}

int32_t win_tick(uint8_t *win)
{
    int32_t r = 0;
    if (ld32(win, 4) == 0) return 0;
    if (ld32(win, 0x54) < 1000 && ld32(win, 0x58) == 0) return 0;
    for (uint16_t i = 0; i < ld16(win, 0x4e); i++) {
        int32_t v = ctrl_tick(((uint8_t **)ldp(win, 0x48))[i]);
        if (i == ld16(win, 0x4e) - 1) r = v;
    }
    return r;
}

int32_t msg_update(input_mgr *in, uint32_t now, uint8_t *win, int32_t sound, int32_t freq, int32_t skip,
                   int32_t *autoc)
{
    uint8_t *last = win_last(win);
    if (ld32(last, 0x20) != 0) st32(last, 0x24, autoc != NULL);
    if (skip == 0) {
        if (input_poll_consume(in, now, 36)) ctrl_key(win_last(win), 9);
        if (input_poll_consume(in, now, 39)) ctrl_key(win_last(win), 10);
        if (in->axis_held[6] != 0) ctrl_key(win_last(win), 8);
    }
    int32_t r = 0;
    if (ld32(win, 4) != 0 && (ld32(win, 0x54) >= 1000 || ld32(win, 0x58) != 0))
        for (uint16_t i = 0; i < ld16(win, 0x4e); i++) {
            int32_t v = ctrl_tick(((uint8_t **)ldp(win, 0x48))[i]);
            if (i == ld16(win, 0x4e) - 1) r = v;
        }
    switch (r) {
    case 5: case 6: {   /* the typing blip */
        if (sound == 0) return 0;
        if (ld32(ldp(sysp(0x320), 0), 0x200) == 0) return 0;
        wl_voice *v = vpool_take(sysp(0x384), 0);
        if (v == NULL) return 0;
        g_eng.dsb_volume_5bb870(v->buf, ld32(ldp(sysp(0x320), 0), 0x210));
        g_eng.dsb_freq_5bb890(v->buf, freq);
        g_eng.dsb_play_5bb8c0(v->buf);
        return 0;
    }
    case 7:   /* waiting to close: count the auto-advance down, then advance */
        if (autoc == NULL) return 0;
        if (sysp(0xaa8) != NULL && jobs_busy(sysp(0xaa8))) return 0;
        if (*autoc > 0) {
            (*autoc)--;
            return 0;
        }
        ctrl_key(win_last(win), 9);
        return 0;
    case 8: return 1;
    default: return 0;
    }
}

int32_t ctrl_input(input_mgr *in, uint8_t *c, uint32_t now, int32_t cue, int32_t nocancel, int32_t ud_ends,
                   int32_t horiz)
{
    int32_t r = 0;
    if (horiz != 0) {   /* left / right move the cursor, held ones repeat */
        if (!input_poll_consume(in, now, 2) || (r = ctrl_key(c, 0)) == 0)
            r = ctrl_key(c, in->axis_held[2] != 0 ? 6 : 7);
        int found = input_poll_consume(in, now, 4);
        if (found) r = ctrl_key(c, 1);
        if (r == 0) r = ctrl_key(c, in->axis_held[3] != 0 ? 4 : 5);
    } else {   /* left / right turn the page, up / down move, held ones repeat */
        if (input_poll_consume(in, now, 2)) r = ctrl_key(c, 2);
        if (input_poll_consume(in, now, 4)) r = ctrl_key(c, 3);
        if (input_poll_consume(in, now, 1)) {
            r = ctrl_key(c, 0);
            if (ud_ends != 0) r = 1;
        }
        if (r == 0) r = ctrl_key(c, in->axis_held[0] != 0 ? 6 : 7);
        if (input_poll_consume(in, now, 3)) {
            r = ctrl_key(c, 1);
            if (ud_ends != 0) {
                r = 1;
                goto keys;
            }
        }
        if (r == 0) r = ctrl_key(c, in->axis_held[1] != 0 ? 4 : 5);
    }
keys:
    if (input_poll_consume(in, now, 36)) r = ctrl_key(c, 9);
    if (nocancel == 0 && input_poll_consume(in, now, 39)) r = ctrl_key(c, 10);
    switch (r) {
    case 1: case 2:
        sfx_play(g_eng.sys, 9, 0, 0);
        return r;
    case 3:
        if (ld32((uint8_t *)ldp(c, 0x17c) + 16 * ld32(ldp(c, 0x174), 0x14), 8) == 0) {   /* a disabled row */
            sfx_play(g_eng.sys, 6, 0, 0);
            return 0;
        }
        if (cue != 0) sfx_play(g_eng.sys, 5, 0, 0);
        return r;
    case 4:
        sfx_play(g_eng.sys, 7, 0, 0);
        return r;
    default: return r;
    }
}

int32_t jobs_busy(const uint8_t *j)
{
    for (uint16_t i = 0; i < ld16(j, 4); i++)
        if (ld32(ldp(j, 8), 24u * i + 0x10) == 2) return 1;
    return 0;
}

int32_t menu_pace(uint8_t *p)
{
    uint32_t now = g_eng.tick();
    int32_t st = ld32(p, 0xc);
    st32(p, 0x20, (int32_t)now);
    if (st == 0) {
        st32(p, 0, (int32_t)now);
        g_eng.h5b1030();
        st32(p, 0xc, 2);
    } else if (st == 1) {   /* owe the elapsed time (100 ms at most); update once a step is owed */
        uint32_t owed = (uint32_t)ld32(p, 4) + now - (uint32_t)ld32(p, 0);
        if (owed > 100) owed = 100;
        st32(p, 4, (int32_t)owed);
        st32(p, 0, (int32_t)now);
        g_eng.h5b1030();
        if ((uint32_t)ld32(p, 4) > (uint32_t)ld32(p, 0x10)) st32(p, 0xc, 2);
    } else if (st == 2) {   /* pay a step per update; render when it runs out */
        uint32_t el = now - (uint32_t)ld32(p, 8), owed = (uint32_t)ld32(p, 4), step = (uint32_t)ld32(p, 0x10);
        st32(p, 0x1c, (int32_t)el);
        if (el <= owed) {
            st32(p, 4, (int32_t)(owed - step));
            if (owed - step <= step) st32(p, 0xc, 1);
        } else {
            st32(p, 4, 0);
            st32(p, 0xc, 1);
        }
    }
    int32_t r = ld32(p, 0xc);
    if (r == 1) {
        uint32_t t = (uint32_t)ld32(p, 0x20);
        if (t - (uint32_t)ld32(p, 0x14) <= 1000) {
            st32(p, 0x18, ld32(p, 0x18) + 1);
        } else {
            st32(p, 0x24, ld32(p, 0x18));
            st32(p, 0x18, 0);
            st32(p, 0x14, (int32_t)t);
        }
    } else if (r == 2) {
        st32(p, 8, ld32(p, 0x20));
    }
    return r;
}

int32_t menu_run(uint8_t *m, uint8_t *c, uint8_t *msgwin, int32_t cue, int32_t lr, int32_t hot17,
                 int32_t world, int32_t anykey, int32_t ud_ends, int32_t ud)
{
    wl_roomloop *rl = ldp(m, 4);
    input_mgr *in = ldp(m, 8);
    if (world != 0)
        for (int i = 0; i < 32; i++)
            if (ld32(g_world->chars[i], 0x1d0) != 0) cmd_clear(&g_world->chars[i]->cmd);
    uint8_t pace[0x28];
    st32(pace, 0x10, 16);
    st32(pace, 0, 0);
    st32(pace, 4, 17);
    st32(pace, 8, 0);
    st32(pace, 0x1c, 0);
    st32(pace, 0xc, 0);
    st32(pace, 0x24, 0);
    st32(pace, 0x18, 0);
    st32(pace, 0x14, (int32_t)g_eng.tick());
    for (;;) {
        int32_t s = menu_pace(pace);
        if (s == 1) {   /* render + Flip */
            g_eng.h48c150(rl, (intptr_t)ldp(sysp(0x286c), 0x16c), 0, (intptr_t)pace);
            g_eng.flip_5b8fc0(sysp(0x286c), (intptr_t)sysp(0x288c));
            continue;
        }
        if (s != 2) continue;
        uint32_t now = (uint32_t)ld32(pace, 0x20);
        timed_sounds_tick(g_eng.sys);
        g_eng.h40fe00(g_eng.sys);
        for (uint16_t j = 0; j < rl->nrooms; j++) {
            room_input_update(&rl->rooms[j]);
            input_mgr *ri = rl->rooms[j].in;
            if (ri != NULL && input_poll_consume(ri, now, 19)) rl->f34 = 1;
        }
        if (input_poll_consume(in, now, 34)) return 6;
        if (hot17 != 0 && input_poll_consume(in, now, 17)) return 20;
        if (input_poll_consume(in, now, 16)) {
            sfx_play(g_eng.sys, 7, 0, 0);
            return 8;
        }
        if (lr != 0) {
            if (input_poll_consume(in, now, 2)) return 16;
            if (input_poll_consume(in, now, 4)) return 17;
        }
        if (ud != 0) {
            if (input_poll_consume(in, now, 1)) return 18;
            if (input_poll_consume(in, now, 3)) return 19;
        }
        if (c != NULL)
            switch (ctrl_input(in, c, now, cue, 0, ud_ends, 0)) {
            case 1: case 2: return 13;
            case 3: return 12;
            case 4:
                sfx_play(g_eng.sys, 7, 0, 0);
                return 11;
            default: break;
            }
        if (anykey != 0 && (input_poll_consume(in, now, 39) || input_poll_consume(in, now, 36))) {
            sfx_play(g_eng.sys, 5, 0, 0);
            return 0;
        }
        if (msgwin != NULL && msg_update(in, now, msgwin, 1, 0, 0, NULL) != 0) return 12;
        for (uint16_t j = 0; j < ld16(rl->fe8, 6); j++) {
            uint8_t *w = ((uint8_t **)ldp(rl->fe8, 0))[j];
            if (w != msgwin) win_tick(w);
        }
        if (world != 0) world_tick(rl, 1);
        g_eng.h499ab0(rl);
    }
}
