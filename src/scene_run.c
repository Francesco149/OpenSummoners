/* scene_run.c — the scene command runner 0x439690.  See scene_run.h.  Transcribed from the asm (the runner's
 * offsets are raw: IDA's cls_401D40 names its fields from +0x2e0 up 4 bytes low, quirk #144). */
#include "scene_run.h"

#include <stddef.h>
#include <string.h>

#include "camera_follow.h"
#include "char_ai.h"
#include "screen_fx.h"
#include "engine_calls.h"
#include "entity_life.h"
#include "event.h"          /* sys_unload 0x43e3a0 */
#include "gimmick.h"        /* room_input_flush 0x411560 */
#include "input.h"
#include "rng.h"
#include "room_load.h"      /* volume_fade 0x43c9b0 */
#include "room_loop.h"
#include "scene_box.h"
#include "scene_cast.h"
#include "sound_fx.h"
#include "ui_core.h"
#include "ui_run.h"
#include "ui_text.h"
#include "world_tick.h"

static int32_t  ld32(const void *p, uint32_t off) { int32_t v; memcpy(&v, (const uint8_t *)p + off, 4); return v; }
static uint16_t ld16(const void *p, uint32_t off) { uint16_t v; memcpy(&v, (const uint8_t *)p + off, 2); return v; }
static void st32(void *p, uint32_t off, int32_t v) { memcpy((uint8_t *)p + off, &v, 4); }
static void st16(void *p, uint32_t off, uint16_t v) { memcpy((uint8_t *)p + off, &v, 2); }
static void *ldp(const void *p, uint32_t off) { void *v; memcpy(&v, (const uint8_t *)p + off, sizeof v); return v; }
static void stp(void *p, uint32_t off, const void *v) { memcpy((uint8_t *)p + off, &v, sizeof v); }
static void *sysp(uint32_t off) { return ldp(g_eng.sys, off); }
static const void *D(uint32_t va) { return g_eng.data(va); }
static const uint8_t *cfg(void) { return ldp(sysp(0x320), 0); }
#define IP(p) ((intptr_t)(p))

/* the box records: the room loop's +0x54 (current) / +0x90 (previous) */
static void box_retire(uint8_t *L)
{
    if (ldp(L, 0x90) != NULL) {
        st32(ldp(L, 0x90), 0x50, 0);
        memset(L + 0x90, 0, 0x3c);
    }
    memcpy(L + 0x90, L + 0x54, 0x3c);
    memset(L + 0x54, 0, 0x3c);
}

/* the id (+4) of the list's cursor row */
static int32_t cur_id(const uint8_t *c)
{
    return ld32((const uint8_t *)ldp(c, 0x17c) + 16u * (uint32_t)ld32(ldp(c, 0x174), 0x14), 4);
}

/* a window style {sprite, 10 frames, 32, 32, 0} */
static void style(uint8_t *st, int32_t sprite, uint16_t f0)
{
    const uint16_t fr[10] = { f0, 0, 1, 2, 3, 4, 5, 6, 7, 8 };
    st32(st, 0, sprite);
    memcpy(st + 4, fr, sizeof fr);
    st32(st, 0x18, 32);
    st32(st, 0x1c, 32);
    st32(st, 0x20, 0);
}

/* the camera target (x, y) clamped to the map */
static void cam_target(camera_view *v, int32_t x, int32_t y)
{
    int32_t mx = v->map_w - v->vp_w, my = v->map_h - v->vp_h;
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    v->tgt_x = mx < x ? mx : x;
    v->tgt_y = my < y ? my : y;
}

static const uint16_t CUR4[4] = { 0, 1, 2, 3 };

/* the message box for the runner's new message (R+0x7c the kind) */
static void box_build(uint8_t *L, uint8_t *r, uint32_t voice)
{
    void *sys = g_eng.sys;
    uint8_t st[0x24];
    int32_t kind = ld32(r, 0x7c);
    if (kind >= 0 && (kind <= 2 || kind == 4)) {
        /* a balloon (or the bottom strip): the portrait, the name, the text */
        int32_t y = 0, w = 368, name_x = 0, tx = 28, ty = 20, tw = 44, tl = 3, px = -24, py = -72, nx, ny,
                face = 0;
        if (ld16(r, 0x2f0) != 0) {
            ny = 20; tx = 176; ty = 48; w = 640; tl = 2; tw = 60; y = 368; nx = 176; face = -1; px = 0; py = -32;
            sys_box_style(sys, st);
        } else {
            if (ld32(r, 0x84) != 0) { w = 408; tw = 36; tx = 136; }
            if (strlen((const char *)r + 0x28a) <= 12) { name_x = w - 128; face = 1; }
            else { name_x = w - 192; face = 0; }
            nx = name_x + 20;
            ny = -9;
            sys_balloon_style(sys, st);
        }
        uint8_t *win = win_mgr_open(ldp(L, 0xe8), 0, y, w, 112, 10, st);
        stp(L, 0x54, win);
        stp(L, 0x5c, win_grid_add(ldp(L, 0x54), 0, 0, 0, 0));
        stp(L, 0x60, win_grid_add(ldp(L, 0x54), 0, 0, 0, 0));
        stp(L, 0x6c, win_grid_add(ldp(L, 0x54), name_x, -32, 0, 0));
        stp(L, 0x70, win_grid_add(ldp(L, 0x54), nx, ny, 0, 0));
        stp(L, 0x68, win_grid_add(ldp(L, 0x54), px, py, 0, 0));
        stp(L, 0x64, win_grid_add(ldp(L, 0x54), px, py, 0, 0));
        if (face >= 0) {
            cell_icon_set(ldp(L, 0x6c), 0, 0, ld32(sys, 0xbb0), (uint16_t)face, NULL, 0, 0);
            st32(ldp(L, 0x70), 0x180, 0xffffff);
            st32(ldp(L, 0x70), 0x184, 0x455f7b);
        } else {
            st32(ldp(L, 0x70), 0x180, 0xf08080);
        }
        uint8_t *c = win_text_add(ldp(L, 0x54), tx, ty, (uint16_t)tw, (uint16_t)tl);
        stp(L, 0x74, c);
        sys_msg_keys(sys, (int32_t)(intptr_t)ldp(L, 0x54), c);
        if (ld16(r, 0x2f0) != 0) sys_box_cursor(sys, ldp(L, 0x54), ldp(L, 0x74), -8, -8);
        else sys_box_cursor(sys, ldp(L, 0x54), ldp(L, 0x74), -8, 12);
        st32(L, 0x78, ld32(r, 0x2e0));
    } else {
        /* a narration box: the bottom strip (R+0x2f0) or a centred box */
        int strip = ld16(r, 0x2f0) != 0;
        if (strip) style(st, ld32(sys, 0xb88), 64);
        else style(st, ld32(sys, 0xba8), 0);
        uint8_t *win = strip ? win_mgr_open(ldp(L, 0xe8), 0, 392, 640, 88, 4, st)
                             : win_mgr_open(ldp(L, 0xe8), 128, 32, 384, 128, 4, st);
        stp(L, 0x54, win);
        uint8_t *c = win_ctrl_take(win);
        if (c != NULL) {
            if (strip) ctrl_text_init(c, 100, 24, 62, 2);
            else ctrl_text_init(c, 36, 32, 44, 3);
        }
        stp(L, 0x74, c);
        sys_msg_keys(sys, 0, c);
        const uint8_t *bw = ldp(L, 0x54);
        int32_t dx = ld32(bw, 0x14) - ld32(c, 0xc) - 40;
        int32_t dy = ld32(bw, 0x18) - ld32(c, 0x10) - (strip ? 40 : 20);
        ctrl_cursor_set(c, ld32(sys, 0xb8c), 20, CUR4, 4, 10, dx, dy);
    }
    (void)voice;
}

/* the new message's text: name, text with its key tokens, the typewriter speeds */
static void box_text(uint8_t *L, uint8_t *r, uint32_t voice)
{
    char buf[512];
    st32(r, 0x78, 0);
    if (ldp(L, 0x70) != NULL) grid_cell_text(ldp(L, 0x70), 0, 0, (const char *)r + 0x28a);
    uint8_t *c = ldp(L, 0x74), *t = ldp(c, 0x170);
    uint16_t width = ld16(t, 8);
    const char *txt = ctrl_keys_expand(c, (const char *)r + 0x8a, buf);
    text_layout_set(ldp(ldp(c, 0x170), 0), txt, width);
    st16(ldp(c, 0x170), 4, 0);
    st16(ldp(c, 0x170), 6, 0);
    st32(c, 0x1c, 0);
    int32_t spd = ld32(cfg(), 0x248);
    c = ldp(L, 0x74);
    t = ldp(c, 0x170);
    st32(t, 0xc, 1);
    st32(t, 0x10, spd);
    st32(t, 0x20, 2 * spd);
    st32(t, 0x24, 3 * spd);
    st32(t, 0x28, 5 * spd);
    st32(t, 0x14, ld32(r, 0x2dc) == 0);
    st32(t, 0x18, 0);
    st32(c, 0x1c, 0);
    if ((uint16_t)voice != 0 && (uint16_t)voice != 0x7fff) {
        st32(t, 0x10, 6);
        st32(t, 0x20, 18);
        st32(t, 0x24, 24);
        st32(t, 0x28, 36);
    }
}

/* a temporary cast character's release (its carry, links, entity and voice set) */
static void cast_release(wl_char *c)
{
    uint8_t *a = (uint8_t *)c;
    if (ldp(a, 0xc798) != NULL) g_eng.h442920(ldp(a, 0xc798), 0);
    if (ldp(a, 0xc7a4) != NULL) g_eng.h442920(c, 0);
    for (int i = 0; i < 2; i++) {
        if (ldp(a, 0xc824u + 4u * (uint32_t)i) == NULL) continue;
        entity_release(ldp(a, 0xc824u + 4u * (uint32_t)i), 1);
        stp(a, 0xc824u + 4u * (uint32_t)i, NULL);
    }
    g_eng.h4428f0(c);
    entity_release(&c->base, 0);
    if (ldp(a, 0x15b54) != NULL) {
        g_eng.h468970(ldp(a, 0x15b54), 1);
        stp(a, 0x15b54, NULL);
    }
}

int32_t scene_run(wl_roomloop *rl, wl_event *ev)
{
    void *sys = g_eng.sys;
    uint8_t *r = (uint8_t *)ev, *L = (uint8_t *)rl;
    int32_t autoc = 0, res = 0, freq = 0, blip = 1, high = 0, voiced = 0;
    int32_t *autop = NULL;
    uint8_t *cwin = NULL, *clist = NULL;
    int32_t skip = g_world->f27ac != NULL;
    uint32_t voice = 0;   /* (retail: a dword whose low half is the voice id) */
    input_mgr *in = g_world->room->in;
    g_eng.h40fe00(sys);
    for (uint16_t i = 0; i < rl->nrooms; i++) room_input_flush(&rl->rooms[i]);
    if (ld32(r, 0x20) == 0) goto out;

    /* ── the beat's set-up ── */
    st16(L, 0x30, ld16(r, 0x2f0));
    {   /* the letterbox */
        int32_t lb = ld32(r, 0x6c);
        if (lb >= 1 && lb <= 4) {
            if (lb == 4) st32(L, 0x40, 0);
            else { st32(L, 0x40, 1); st32(L, 0x50, lb == 1 ? 0 : lb == 2 ? -64 : 64); }
            if (ld32(r, 0x70) == 0) {
                if (ld32(L, 0x40) != 0) { st32(L, 0x44, 64); st32(L, 0x48, 64); st32(L, 0x4c, ld32(L, 0x50)); }
                else { st32(L, 0x44, 0); st32(L, 0x48, 0); st32(L, 0x4c, 0); }
            }
        }
        st32(r, 0x6c, 0);
    }
    if (ld32(r, 0x2e4) != 0) {
        box_retire(L);
        sys_unload(sys, 5);
        st32(r, 0x2e4, 0);
    }
    {   /* every timed sound still starting / playing fades out */
        uint8_t *jobs = sysp(0xaa8);
        if (jobs != NULL)
            for (uint16_t k = 0; k < ld16(jobs, 4); k++) {
                uint8_t *j = (uint8_t *)ldp(jobs, 8) + 24u * k;
                if (ld32(j, 0) != 0 && ld32(j, 0x10) < 3) {
                    st32(j, 0x10, 3);
                    st32(j, 0x14, 400);
                }
            }
    }
    if (ld32(r, 0x78) != 0) {   /* a new message */
        uint32_t prev = 0;
        if (ld32(r, 0x7c) == 2) blip = 0;
        else prev = (uint32_t)(freq = ld32(r, 0x80));
        voice = (prev & 0xffff0000u) | ld16(r, 0x2e8);
        st16(r, 0x2e8, 0);
        if ((uint16_t)voice != 0) blip = 0;
        if (ldp(L, 0x54) != NULL && (ld32(L, 0x8c) == 0 || ld16(r, 0x2f0) == 0) &&
            (ld32(L, 0x58) != ld32(r, 0x7c) || ld32(L, 0x78) != ld32(r, 0x2e0) ||
             (ld32(L, 0x7c) == 0 && ld32(r, 0x84) != 0))) {
            box_retire(L);
            sys_unload(sys, 5);
        }
        if (ldp(L, 0x54) == NULL) {
            memset(L + 0x54, 0, 0x3c);
            st32(L, 0x58, ld32(r, 0x7c));
            st32(L, 0x8c, ld16(r, 0x2f0));
            box_build(L, r, voice);
        }
        box_text(L, r, voice);
    }
    if (ldp(L, 0x54) != NULL && ld32(L, 0x58) >= 0 && ld32(L, 0x58) <= 2) {   /* the portrait */
        int32_t bank = ld32(r, 0x84);
        if (bank != 0) {
            if (bank != ld32(L, 0x7c) || ld16(r, 0x88) != ld16(L, 0x80)) {
                cell_icon_set(ldp(L, 0x64), 0, 0, ld32(sys, 0xaacu + 4u * (uint32_t)bank), ld16(r, 0x88), NULL, 0, 0);
                cell_icon_alpha(ldp(L, 0x64), 0, 0, ld32(sys, 0x27a8));
                if (ld32(L, 0x7c) != 0) {
                    cell_icon_set(ldp(L, 0x68), 0, 0, ld32(sys, 0xaacu + 4u * (uint32_t)ld32(L, 0x7c)), ld16(L, 0x80), NULL,
                                  0, 0);
                    cell_icon_alpha(ldp(L, 0x68), 0, 0, 0);
                } else {
                    cell_icon_set(ldp(L, 0x68), 0, 0, 0, 0, NULL, 0, 0);
                }
                if (ldp(L, 0x54) != NULL && ld16(L, 0x82) != 1) {
                    st16(L, 0x82, 1);
                    st32(L, 0x88, 50);
                    st32(L, 0x84, 0);
                }
                st32(L, 0x7c, ld32(r, 0x84));
                st16(L, 0x80, ld16(r, 0x88));
            }
        } else {
            if (ld32(L, 0x7c) != 0 && ldp(L, 0x54) != NULL && ld16(L, 0x82) != 2) {
                st16(L, 0x82, 2);
                st32(L, 0x88, 100);
            }
            if (ld32(L, 0x8c) != 0) st32(L, 0x7c, 0);
        }
    }
    if (ldp(L, 0x90) != NULL && ld32(L, 0xb8) != 0 && ldp(L, 0x90) != NULL && ld16(L, 0xbe) != 3) {
        st16(L, 0xbe, 3);   /* the previous box's portrait fades out */
        st32(L, 0xc4, 100);
    }
    if (ld32(r, 0x84) != 0) high = 1;
    if (ld32(r, 0x24) != 0) {   /* the screen effect (world+0x1040): a cell grid reset, a random phase */
        uint8_t *fx = g_world->f1040;
        int32_t a2c = ld32(r, 0x2c);
        st32(fx, 0xc, ld32(r, 0x28));
        int32_t rr = (int32_t)rng_rand();
        st16(fx, 0x14, 0);
        st32(fx, 0x18, 0);
        st32(fx, 0x10, 3 * rr / 0x8000);
        st32(fx, 0x1c, a2c);
        st16(fx, 0xa, ld16(fx, 8));
        for (uint16_t i = 0; i < ld16(fx, 4); i++)
            for (uint16_t j = 0; j < ld16(fx, 6); j++) {
                uint8_t *e = (uint8_t *)ldp(fx, 0) + 12u * ((uint32_t)ld16(fx, 4) * j + i);
                st16(e, 0, 0);
                st16(e, 2, 0);
                st32(e, 4, i);
                st32(e, 8, j);
            }
    }
    if (ld32(r, 0x30) != 0) {   /* the fades */
        st32(r, 0x30, 0);
        if (ld32(r, 0x38) != 0) {
            uint8_t *fx = g_world->f1040;
            st32(fx, 0x20, ld32(r, 0x38));
            st32(fx, 0x24, 0);
            volume_fade(fx, 1);
        }
        uint8_t *fx = g_world->f1040;
        st32(fx, 0x20, ld32(r, 0x34));
        st32(fx, 0x24, ld32(r, 0x3c));
        volume_fade(fx, 1);
    }
    if (ld32(r, 0x40) != 0) {   /* the camera snaps */
        camera_view *v = g_world->view;
        cam_target(v, ld32(r, 0x44), ld32(r, 0x48));
        v->cap = 0;
        v->flag = 0;
        v = g_world->view;
        v->cur_x = v->tgt_x;
        v->cur_y = v->tgt_y;
        v->vel_x = 0;
        v->vel_y = 0;
        st32(r, 0x40, 0);
    }
    if (ld32(r, 0x4c) != 0) {   /* ... or pans */
        camera_view *v = g_world->view;
        int32_t spd = ld32(r, 0x58);
        cam_target(v, ld32(r, 0x50), ld32(r, 0x54));
        v->cap = spd;
        v->flag = 0;
        st32(r, 0x4c, 0);
    }
    g_world->view->ear_rel = 1;
    g_world->view->ear_x = ld32(r, 0x64);
    g_world->view->ear_y = ld32(r, 0x68);
    uint16_t nch = ld16(r, 0x584);
    if (nch != 0) {   /* the choices */
        uint8_t st[0x24];
        style(st, ld32(sys, 0xba8), 0);
        cwin = win_mgr_open(ldp(L, 0xe8), ld32(r, 0x598), ld32(r, 0x59c), ld32(r, 0x594), 28 * nch + 64, 1, st);
        uint16_t n = ld16(r, 0x584);
        uint8_t *c = win_ctrl_take(cwin);
        if (c != NULL) ctrl_grid_init(c, 40, 32, n, 1, n, 2);
        clist = c;
        ctrl_cursor_set(clist, ld32(sys, 0xb8c), 16, CUR4, 4, 16, -32, -30);
        if (ld32(cfg(), 0x254) == 0) {
            st32(clist, 0x14, 1);
            st32(clist, 0x18, -9);
        }
        for (uint16_t k = 0; k < ld16(r, 0x584); k++)
            grid_row_text_add(clist, ld32(r, 0x5f0u + 0x58u * k), ld32(r, 0x5f4u + 0x58u * k),
                              (const char *)r + 0x5a0u + 0x58u * k);
        uint8_t *gi = ldp(clist, 0x174);
        int32_t pick = ld32(r, 0x588);
        for (uint16_t k = 0; (int32_t)k < ld32(gi, 0x10); k++) {
            const uint8_t *row = (const uint8_t *)ldp(clist, 0x17c) + 16u * k;
            if (ld32(row, 0) == 0 && ld32(row, 4) == pick) {
                st32(gi, 0x14, k);
                grid_page_fit(clist);
                break;
            }
        }
    }
    if (ldp(r, 0xb28) != NULL) {   /* the picture */
        st32(ldp(r, 0xb28), 0xc, ld32(L, 0xe8));
        g_eng.h43d540(ldp(r, 0xb28));
    }
    room_views_mark(rl, ld32(r, 0x74));
    st16(L, 0x38, 0);
    st32(L, 0x3c, ld32(r, 0xb20));
    st16(L, 0x3a, 0);
    if (g_world->f27ac != NULL) {   /* the auto-advance */
        autoc = sysp(0xaa8) != NULL && (uint16_t)voice != 0 ? 40 : 50;
        autop = &autoc;
    } else if (sysp(0xaa8) != NULL && (uint16_t)voice != 0 && ld32(cfg(), 0x1e4) == 1) {
        autoc = (uint16_t)voice != 0x7fff ? 40 : 60;
        autop = &autoc;
    }

    /* ── the frame loop ── */
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
        if (s == 1) {   /* render (+ the letterbox fill, the demo captions) + Flip */
            void *paint = ldp(sysp(0x286c), 0x16c);
            g_eng.h48c150(rl, IP(paint), 0, IP(pace));
            if (ld32(r, 0xb24) != 0 && (g_world->tick15 & 1) == 0) {
                void *hdc = NULL;
                g_eng.h5b94e0(ldp(sysp(0x286c), 0x16c), IP(&hdc));
                g_eng.select_obj(hdc, ldp(ldp(sysp(0x274c), 0), 0));
                g_eng.select_obj(hdc, ldp(ldp(sysp(0x2750), 0), 0));
                g_eng.h5b5ef0((void *)D(0x8a6610u), IP(hdc), 0, 0, g_world->view->vp_w / 100, g_world->view->vp_h / 100);
                g_eng.h5b9500(ldp(sysp(0x286c), 0x16c), IP(hdc));
            }
            const uint8_t *wt = g_world->f27ac;
            if (wt != NULL && ld32(wt, 0x64) != 0) {
                const char *l1 = D(0x8552acu), *l2 = D(0x855294u);
                g_eng.h43e250(2, ldp(sysp(0x286c), 0x16c), l1, 320 - (int32_t)((strlen(l1) * 4) >> 1), 430, 0);
                g_eng.h43e250(2, ldp(sysp(0x286c), 0x16c), l2, 320 - (int32_t)((strlen(l2) * 4) >> 1), 450, 0);
            }
            g_eng.flip_5b8fc0(sysp(0x286c), IP(sysp(0x288c)));
            continue;
        }
        if (s != 2) continue;
        uint32_t now = (uint32_t)ld32(pace, 0x20);
        timed_sounds_tick(sys);
        g_eng.h40fe00(sys);
        for (uint16_t j = 0; j < rl->nrooms; j++) {
            room_input_update(&rl->rooms[j]);
            input_mgr *ri = rl->rooms[j].in;
            if (ri != NULL && input_poll_consume(ri, now, 19)) rl->f34 = 1;
        }
        {   /* the BGM ducks while a voice line plays */
            uint8_t *jobs = sysp(0xaa8);
            void *zdm = sysp(0x2884);
            if (jobs != NULL && zdm != NULL) {
                int playing = 0;
                for (uint16_t k = 0; k < ld16(jobs, 4); k++)
                    if (ld32((uint8_t *)ldp(jobs, 8) + 24u * k, 0x10) == 2) { playing = 1; break; }
                if (playing && ld32(r, 0x2ec) == 0) g_eng.h5bcfb0(zdm, -300, 50, 0);
                else if (g_eng.h5bcfa0(sysp(0x2884)) < 0) g_eng.h5bcfb0(sysp(0x2884), 0, 25, 50);
            }
        }
        if (in != NULL) {   /* the skip key */
            for (int32_t k = 63; k >= 0; k--) {
                uint8_t *e = ldp(in, 0xcu + 4u * (uint32_t)k);
                int hit = g_world->f27ac != NULL ? ld32(e, 0) != 0 : ld32(e, 0) == 34;
                if (!hit || ld32(e, 8) != 1 || now - (uint32_t)ld32(e, 4) > 100) continue;
                res = 6;
                st32(ldp(in, 0xcu + 4u * (uint32_t)k), 0, 0);
                if (g_world->f27ac != NULL) st32(g_world->f27ac, 0x78, 1);
                goto out;
            }
        }
        for (int k = 0; k < 32; k++) {   /* the cast */
            uint8_t *rec = r + 0x2fcu + 20u * (uint32_t)k;
            if (ld32(rec, 0) == 0 || cast_step(ldp(rec, 0xc)) == 0) continue;
            if (ld32(rec, 0x10) != 0) cast_release(ldp(rec, 0xc));
            st32(rec, 0, 0);
        }
        if (ld32(r, 0x580) != 0) {   /* a sound */
            wl_voice *v = ld32(g_world->grid, 0x20) != 0 ? vpool_take(sysp(0x3c0), 0) : vpool_take(sysp(0x3bc), 0);
            if (v != NULL) {
                g_eng.dsb_volume_5bb870(v->buf, ld32(cfg(), 0x168) + 2000);
                g_eng.dsb_freq_5bb890(v->buf, -2000);
                g_eng.dsb_play_5bb8c0(v->buf);
            }
            st32(r, 0x580, 0);
        }
        uint8_t *pic = ldp(r, 0xb28);
        if (pic != NULL) {   /* the picture: its list, then its message */
            if (ld16(pic, 0x10) == 1) {
                win_tick(ldp(pic, 0));
                if (ctrl_input(in, ldp(pic, 4), now, 1, 1, 0, 0) == 3) {
                    st32(pic, 0x1c, cur_id(ldp(pic, 4)));
                    for (uint16_t k = 1; k < ld16(pic, 0x12); k++) {
                        uint8_t *e = pic + 48u * k;
                        if (ld32(e, 0x14) != 0) st32(e, 0x1c, g_eng.h43d4b0(pic, IP(e + 0x3c), ld32(e, 0x1c)));
                    }
                    g_eng.h43dd30(pic, IP(pic + 0x14), ld16(pic, 0x12));
                    st32(ldp(pic, 0), 0x50, 0);
                    g_eng.h43d830(pic);
                    st16(pic, 0x10, (uint16_t)(ld16(pic, 0x10) + 1));
                }
            } else if (ld16(pic, 0x10) == 2 && msg_update(in, now, ldp(pic, 0), 1, 0, 0, NULL) != 0) {
                st32(ldp(pic, 0), 0x50, 0);
                st16(pic, 0x10, 1);
                stp(r, 0xb28, NULL);
                goto out;
            }
        }
        if (cwin != NULL) {   /* the choices */
            if (ld32(cwin, 4) != 0 && (ld32(cwin, 0x54) >= 1000 || ld32(cwin, 0x58) != 0))
                for (uint16_t i = 0; i < ld16(cwin, 0x4e); i++) ctrl_tick(((uint8_t **)ldp(cwin, 0x48))[i]);
            int32_t cr = ctrl_input(in, clist, now, 1, ld32(r, 0x590), 0, 0);
            if (cr == 3 || cr == 4) {
                st32(r, 0x588, cr == 3 ? cur_id(clist) : ld32(r, 0x58c));
                if (ld32(r, 0x20) == 5) goto out;
            }
        }
        if (ldp(L, 0x90) != NULL) {   /* the previous box fades */
            box_fade_tick(L + 0x90);
            if (ld16(L, 0xbe) != 3) {
                st32(ldp(L, 0x90), 0x50, 0);
                memset(L + 0x90, 0, 0x3c);
            }
        }
        uint8_t *bw = ldp(L, 0x54);
        if (bw != NULL) {   /* the box: its voice line once shown, placement, fade, update */
            uint8_t *jobs = sysp(0xaa8);
            void *vmod = sysp(0x310);
            if (jobs != NULL && (uint16_t)voice != 0 && vmod != NULL && (uint16_t)voice != 0x7fff &&
                ld32(bw, 0x54) == 1000 && ld32(bw, 4) != 0 && voiced == 0) {
                voiced = 1;
                g_eng.h43c1b0(jobs, IP(vmod), (intptr_t)voice, ld32(cfg(), 0x1bc));
            }
            if (ld16(r, 0x2f0) == 0) {
                switch (ld32(L, 0x58)) {
                case 0:
                    box_place((int32_t)(intptr_t)r, ldp(L, 0x54), ld32(L, 0x74), ldp(L, 0x5c), ldp(L, 0x60), ldp(L, 0x78), 0,
                              high);
                    break;
                case 1:
                case 4:
                    box_place((int32_t)(intptr_t)r, ldp(L, 0x54), ld32(L, 0x74), ldp(L, 0x5c), ldp(L, 0x60), NULL, 0, 0);
                    break;
                case 2:
                    box_place((int32_t)(intptr_t)r, ldp(L, 0x54), ld32(L, 0x74), ldp(L, 0x5c), ldp(L, 0x60), ldp(L, 0x78), 1,
                              high);
                    break;
                default:
                    break;
                }
            }
            box_fade_tick(L + 0x54);
            if (msg_update(in, now, ldp(L, 0x54), blip, freq, skip, autop) != 0 && ld32(r, 0x20) == 1) goto out;
        }
        if (ld32(r, 0x57c) > 0) st32(r, 0x57c, ld32(r, 0x57c) - 1);
        if (ldp(r, 0x60) != NULL) {   /* the listener follows an actor */
            const wl_body *b = ((const wl_entity *)ldp(r, 0x60))->body;
            camera_view *v = g_world->view;
            v->ear_rel = 0;
            v->ear_x = b->act_78 + b->x + ld32(r, 0x64);
            v->ear_y = b->y + ld32(r, 0x68);
        }
        if (ld32(r, 0x1c) == 0) world_tick(rl, 1);
        if (ldp(r, 0x5c) != NULL) {   /* ... and the camera */
            const wl_body *b = ((const wl_entity *)ldp(r, 0x5c))->body;
            camera_view *v = g_world->view;
            int32_t spd = ld32(r, 0x58);
            cam_target(v, b->act_78 + b->x + ld32(r, 0x50) - 30000, b->y + ld32(r, 0x54) - 27200);
            v->cap = spd;
            v->flag = 0;
        }
        g_eng.easer_43d1d0(g_world->view);
        screen_step(rl);
        if (ld32(g_world->f1040, 0x18) != 0) st32(r, 0x24, 0);
        switch (ld32(r, 0x20)) {   /* the wait mode */
        case 2:
            if (ld32(r, 0x24) == 0) goto out;
            break;
        case 3:
            if (g_world->view->cur_x == g_world->view->tgt_x && g_world->view->cur_y == g_world->view->tgt_y) goto out;
            break;
        case 4: {
            int busy = 0;
            for (uint16_t k = 0; k < 32; k++)
                if (ld32(r, 0x2fcu + 20u * k) != 0 && ld32(r, 0x300u + 20u * k) == 1) { busy = 1; break; }
            if (!busy) goto out;
            break;
        }
        case 6:
            if (ld32(r, 0x57c) <= 0) goto out;
            break;
        case 7:
            if (ld32(L, 0x40) != 0 ? ld32(L, 0x48) >= 64 : ld32(L, 0x48) <= 0) goto out;
            break;
        case 9:
            if (ld32(rl->f11c, 0x20) == 0) goto out;
            break;
        default:
            break;
        }
    }
out:
    if (sysp(0x2884) != NULL && g_eng.h5bcfa0(sysp(0x2884)) < 0) g_eng.h5bcfb0(sysp(0x2884), 0, 25, 50);
    if (cwin != NULL) {
        st32(cwin, 0x50, 0);
        st16(r, 0x584, 0);
    }
    input_repoll_flush(rl);
    return res;
}
