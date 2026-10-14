/* char_apply.c — FUN_00485fc0.  See char_apply.h.
 *
 * Transcribed from the decompile; the 0x5b4d00 calls are thiscall(self, a, b, c)
 * (call-site pushes — IDA adds a phantom register argument).  Labels keep retail's
 * control flow. */
#include "char_apply.h"

#include <stddef.h>
#include <string.h>

#include "char_integrate.h"
#include "engine_calls.h"
#include "map_grid.h"
#include "rng.h"

#define IP(p) ((intptr_t)(p))
#define KEYCFG() (**g_eng.keycfg)

static int32_t grid_i32(size_t off) { int32_t v; memcpy(&v, g_world->grid + off, 4); return v; }
static int32_t hp_sum(const wl_party *p) { return p->hp_max + p->hp_bonus1 + p->hp_bonus2; }
static int32_t mp_sum(const wl_party *p) { return p->mp_max + p->mp_bonus1 + p->mp_bonus2; }

/* one footstep / landing sound (0x485fc0 LABEL_189): by the ground surface under the body */
static void footstep(wl_char *self, wl_body *b, int32_t kind)
{
    if (g_eng.h485f70(self) != 0) return;
    int32_t m = self->f15944;
    if (m <= 0) return;
    if (m <= 2) {
        if (b->slow != 0) return;
        int32_t c = b->c_flag;
        if (c != 0) {
            if (c == 1) g_eng.h489280(self, 0x15, kind, 0, -1500, -500, 1000);
            return;
        }
        int32_t surf;
        if (b->attach != NULL) {
            memcpy(&surf, (const uint8_t *)b->attach + 0x1e0, 4);
        } else {
            int32_t yb = b->h + b->y - 1 + 100;
            surf = g_eng.h487250(g_world->grid, b->x, yb);
            if (surf == 0) surf = g_eng.h487250(g_world->grid, b->x + b->w, yb);
        }
        if (self->f15944 == 2 && surf >= 2 && surf <= 7) {
            g_eng.h489280(self, 0x8c, kind, 0, -1500, 1000, 2000);
            return;
        }
        switch (surf) {
        case 1: g_eng.h489280(self, 0x0f, kind, 0, -1500, -1000, 2000); break;
        case 2: g_eng.h489280(self, 0x10, kind, 0, -1000, -500, 1000); break;
        case 3: g_eng.h489280(self, 0x13, kind, -500, -1500, -1000, 2000); break;
        case 4: g_eng.h489280(self, 0x14, kind, 0, -1000, -500, 1000); break;
        case 5: g_eng.h489280(self, 0x12, kind, 0, -1000, -500, 1000); break;
        case 6: g_eng.h489280(self, 0x11, kind, 0, -1500, 0, 1000); break;
        case 7: g_eng.h489280(self, 0x12, kind, 0, -2000, -1500, 1000); break;
        default: break;
        }
    } else if (m == 3) {
        int32_t pan = kind == 2 ? 0 : -1500;
        int32_t r = (int32_t)rng_rand();
        g_eng.h407ba0(self, 0, self->step_sfx, pan, 500 - 1000 * r / 0x8000, 0);
    }
}

static void leash_release(wl_char *self)
{
    int32_t warp = self->leash_warp;
    wl_body *b = self->base.body;
    self->leash = NULL;
    if (warp != 0) g_eng.h4874b0(self, (int16_t)self->leash_warp, 800);
    else g_eng.h5b4d00(self, 0, 1, b->facing);
}

void char_apply(wl_char *self)
{
    wl_body *b0 = self->base.body;
    int stunned = self->stun_on != 0 && self->stun_t >= self->stun_len;
    if (self->f270 >> 16) self->f270 -= 0x10000u;

    /* ── regen ── */
    if (self->f1f8 == 1 && self->base.faction == 2 && self->f268 == 0) {
        if (self->f200 == 0 || self->f14288 == 1) self->regen_t = 0;
        uint16_t t = self->regen_t;
        if (t >= 0x640u) {
            g_eng.h5b54d0(self, 30 * hp_sum(self->party) / 100, 0, 0);
            g_eng.h5b5580(self, 30 * mp_sum(self->party) / 100, 0, 0);
            self->regen_t = 0;
        } else {
            self->regen_t = (uint16_t)(t + 1);
        }
    }
    self->fc788 = 0;
    if (self->f158c2 != 0) { if (--self->f158c2 == 0) self->f158c4 = 0; }
    if (self->fc784 > 0 && b0->state != 5) self->fc784 -= 5;
    self->fc82c[0] = 0; self->fc82c[1] = 0;
    self->flags204 = (self->flags204 & 0xffffu) | ((((self->flags204 >> 16) + 1) & 0x7fffu) << 16);

    /* ── falling out of the map / hazard tiles ── */
    if (b0->pose < 0x70u || b0->pose > 113u) {
        int32_t mm = self->move_mode;
        if (mm <= 0 || mm > 3) {
            int fell = b0->y > grid_i32(MG_DIM1_PX);
            int32_t kind = 0;
            int32_t pit = g_eng.h487730(self, IP(b0), IP(&kind));
            if (fell || (self->party->busy == 0 && pit != 0)) {
                if (g_world->f1034 == 0 && g_world->scene_kind != 2 && self->f268 == 0 && self->no_sim == 0 &&
                    self->f200 == 0 && g_eng.h5b54d0(self, -5 * hp_sum(self->party) / 100, 0, 0))
                    g_eng.h5b4d00(self, fell, 0, 99);
                if (self->respawn != 0) {
                    if (pit != 0) g_eng.h445fd0(self, 0, kind == 3 ? 0x11 : 0x12, 0, 0, 0);
                    else          g_eng.h445fd0(self, 0, 0xd, 0, 0, 0);
                    g_eng.h407ba0(self, 0, 131, 0, 0, 0);
                    b0->x = self->respawn_x;
                    b0->y = self->respawn_y - b0->h;
                    b0->vel_x = 0; b0->vvel = 0;
                    body_region_flags(b0);
                    body_set_state(b0, 0);
                    body_action_reset(b0);
                    g_eng.h41e180(self, 0);
                    self->invuln_b = 40; self->invuln_a = 40;
                } else if (self->leash != NULL && self->fall_warp != 0) {
                    g_eng.h4874b0(self, (int16_t)self->fall_warp, 800);
                } else if (self->f15904 != 0) {
                    if (fell) {
                        g_eng.h4467d0(self);
                    } else if (pit != 0) {
                        int32_t d = 5 * hp_sum(self->party) / 100;
                        if (d < 1) d = 1;
                        if (g_eng.h5b54d0(self, -d, 0, 0)) g_eng.h5b4d00(self, 0, 0, 99);
                    }
                }
            } else if (self->respawn != 0 && !g_eng.h4593c0(IP(b0)) && self->base.body->attach == NULL) {
                g_eng.h487570(self, IP(b0));
            }
        }
    }
    if (self->base.alive == 0) return;

    int32_t was_slow = self->base.body->slow, was_c = self->base.body->c_flag, was_gr = b0->grounded;
    self->base.tint_f4 = self->base.tint_f8;
    if (self->invuln_b != 0) self->invuln_b--;
    if (self->invuln_a != 0) self->invuln_a--;
    if (self->base.rects != NULL)
        for (uint16_t i = 0; i < self->base.nrects; i++) {
            uint16_t t; memcpy(&t, self->base.rects + 32u * i + 28, 2);
            if (t != 0) { t--; memcpy(self->base.rects + 32u * i + 28, &t, 2); }
        }

    /* ── the voice: a queued line, then the positional volume of every playing voice ── */
    if (self->voice != NULL) {
        if (self->voice_delay > 0 && --self->voice_delay == 0) {
            int32_t id = self->voice_id, vol = self->voice_vol, arg = self->voice_arg;
            if (id != 0 && self->voice != NULL && self->f15b5c == 0 && (self->voice_mode != 1 || KEYCFG()->voices != 0)) {
                int32_t lvl, pan = 0, a6 = 0;
                if (vol != 0) {
                    lvl = KEYCFG()->voice_volume + self->voice_vol_bias;
                    g_eng.h4099a0(self->voice, id, arg, lvl, pan, a6, vol);
                } else if (g_eng.h40a010(self, IP(&lvl), IP(&pan), IP(&a6)) != 0) {
                    g_eng.h4099a0(self->voice, id, arg, lvl, pan, a6, vol);
                }
            }
        }
        int32_t base_vol = self->voice_mode != 0 ? KEYCFG()->voice_volume : KEYCFG()->sfx_volume;
        int32_t bias = self->voice_mode != 0 ? -2000 : 0;
        int32_t vol, pan;
        if (g_eng.sfx_place_40bb90(self, base_vol + self->voice_vol_bias, bias, 0, self->base.body, &vol, &pan) == 0) {
            vol = -10000; pan = 0;
        }
        uint8_t *v = self->voice;
        int32_t v8; memcpy(&v8, v + 8, 4);
        g_eng.h446400(v, v8);
        for (uint16_t i = 0; ; i++) {
            uint16_t n; memcpy(&n, v + 4, 2);
            if (i >= n) break;
            uint8_t *grp; memcpy(&grp, v, sizeof grp);
            grp += 1296u * i;
            uint16_t m; memcpy(&m, grp + 6, 2);
            for (uint16_t k = 0; k < m; k++) {
                uint8_t *e = grp + 40u * k;
                void **buf; memcpy(&buf, e + 28, sizeof buf);
                int32_t e44, e40, e16;
                memcpy(&e44, e + 44, 4); memcpy(&e40, e + 40, 4); memcpy(&e16, e + 16, 4);
                if (buf != NULL && e44 == 0 && e40 == 0) {
                    int32_t lv = vol + e16;
                    if (lv < -10000) lv = -10000;
                    memcpy(e + 32, &lv, 4);
                    g_eng.dsb_volume_5bb870(*buf, lv);
                    g_eng.dsb_pan_5bb880(*buf, pan);
                }
                memcpy(&m, grp + 6, 2);
            }
        }
    }

    {
        int32_t dx = self->fc780 - b0->x;
        if ((dx < 0 ? -dx : dx) > 1600) { self->fc780 = b0->x; self->fc77c = b0->facing; }
    }
    for (int j = 0; j < 8; j++)
        if (self->recent_hit[j].t != 0 && --self->recent_hit[j].t == 0) self->recent_hit[j].id = 0;

    /* ── ally proximity (every 8th tick) ── */
    if (self->fc794 != 0 && ((self->flags204 >> 16) & 7u) == 4u) {
        wl_char *o = (wl_char *)(intptr_t)g_eng.h47e330(self, self->base.faction, 1, 0, 0, 0, 0, 0);
        if (o != NULL) {
            int32_t dx = g_eng.h44e640(self, 0) - g_eng.h44e640(o, 0);
            if (dx < 0) dx = -dx;
            int32_t dy = g_eng.h44e680(self, 0) - g_eng.h44e680(o, 0);
            if (dy < 0) dy = -dy;
            if (dx < 24000 && dy < 12000) g_eng.h5b5600(self, IP(o), 1);
        }
    }

    /* ── idle animations ── */
    if (self->idle_n != 0 && g_world->f2760 == 0 && self->fc794 != 0 && self->party->busy == 0) {
        const wl_cmd *c = &self->cmd;
        if (c->f04 != 0 || c->lr != 0 || c->jump != 0 || c->ud != 0 || c->skill != 0 || c->act != 0) {
            self->idle_t = self->idle_first + (int32_t)((uint32_t)(self->idle_jitter * (int32_t)rng_rand()) >> 15);
        } else if (b0->pose == 0 && (self->move_mode != 0 || b0->grounded != 0) && b0->vel_x == 0 && b0->vvel == 0) {
            if (self->idle_t != 0 && --self->idle_t == 0) {
                body_action_start(b0, (int16_t)self->idle[self->idle_i].pose, 100);
                self->idle_t = self->idle_next + (int32_t)((uint32_t)(self->idle_jitter * (int32_t)rng_rand()) >> 15);
                if (++self->idle_i >= (int32_t)self->idle_n) self->idle_i = 0;
            }
        }
    }

    wl_body *bb = b0;
    if (stunned) {
        uint16_t f = b0->f66;
        body_action_reset(b0);
        b0->f66 = f < 100u ? 0 : (uint16_t)(f - 100);
        goto tail;
    }
    if (self->base.body->f66 != 0) {
        uint16_t lo = (uint16_t)(self->flags204 + 1);
        if (lo > 1000u) lo = (uint16_t)(lo - 1000);
        self->flags204 = (self->flags204 & 0xffff0000u) | lo;
    }
    g_eng.integrate_442a70(self, &self->cmd, self->base.body, self->base.body, 0, 0);
    if (self->base.alive == 0) return;

    if (self->no_sim != 0) {
        for (uint16_t k = 1; k < self->base.nbody; k++) self->base.body[k].active = 0;
    } else if (self->nav_sim_off == 0 || self->f14288 != 0) {
        for (uint16_t k = 1; k < self->base.nbody; k++)
            g_eng.integrate_442a70(self, &self->cmd, &self->base.body[k - 1], &self->base.body[k], 1, k);
    } else if (((self->flags204 >> 16) & 7u) == 1u) {
        for (uint16_t k = 1; k < self->base.nbody; k++) {
            wl_body *d = &self->base.body[k], *s0 = self->base.body;
            d->active = s0->active; d->x = s0->x; d->y = s0->y; d->w = s0->w; d->h = s0->h;
            d->nhit = s0->nhit; d->nocollide = s0->nocollide;
        }
    }

    /* ── water entry / exit splashes ── */
    if (self->no_splash == 0) {
        wl_body *s = self->base.body;
        if (was_slow == 0 || s->slow != 0 || s->vvel >= -5000) {
            if (was_c == 0 && s->c_flag != 0 && s->vvel > 5000) {
                int32_t mat = g_eng.h4872d0(1, IP(s));
                int32_t y = s->h + s->y + 1000;
                g_eng.h448cb0(self, mat == 3 ? 100149 : 100147, 1, 12, 0, y, 0, 0);
            }
        } else if (g_eng.h4872d0(1, IP(s)) == 3) {
            g_eng.h448cb0(self, 100148, 1, 12, 0, s->y + 3000, 0, 0);
        } else {
            g_eng.h448cb0(self, 100146, 1, 12, 0, s->y + 3000, 0, 0);
        }
    }
    if ((uint32_t)bb->slow >= 2u && (self->flags204 & 0x30000u) == 0 && 10 * (int32_t)rng_rand() / 0x8000 == 0) {
        int32_t dy = 800 * (int32_t)rng_rand() / 0x8000 - 800;
        int32_t r = (int32_t)rng_rand();
        g_eng.particle_557370(self, self, 100105, bb, 1600 * r / 0x8000 - 800, dy, 99, 1, 0, 0, 2, 0, 0, 10);
    }
    if (KEYCFG()->effects != 0 && bb->grounded == 1 && bb->w < 6400 && bb->c_flag == 0 && self->move_mode == 0 &&
        bb->state != 1) {
        int32_t v = bb->vel_x;
        if ((v < 0 ? -v : v) > 20000) {
            g_eng.h448cb0(self, 100158, 0, 4, 0, 0, 0, 1);
            if (bb->state < 3u || bb->state > 4u)
                g_eng.h476880(self, 4, 0, 0x88, 800 * (bb->vel_x - 25000) / 15000 - 1200, 0, 0, 0, 10);
        }
    }

    /* ── footsteps ── */
    if (self->step_delay != 0) {
        if (--self->step_delay == 0) footstep(self, bb, self->step_kind);
    } else if (was_gr == 0 && bb->grounded == 1) {
        self->step_delay = 5; self->step_kind = 2;
        footstep(self, bb, 2);
    } else if (bb->clip != NULL && bb->clip_timer == 0) {
        uint16_t k; memcpy(&k, (const uint8_t *)bb->clip + 340 + 2u * bb->clip_frame, 2);
        if (k != 0) {
            int32_t kind = k;
            if (k == 11) { kind = 1; self->step_delay = 5; self->step_kind = 1; }
            footstep(self, bb, kind);
        }
    }

    if (self->f15ab0 == 1) {
        g_eng.h4895c0(self);
        self->f15ab0 = 0;
        if (self->base.alive == 0) return;
    }
    if (self->party->member != 0 && self->party->busy != 0 && self->base.body->pose != self->member_pose) {
        self->party->member = 0;
        if (self->base.code == 50010) {
            self->difficulty = 0;
            g_eng.h41ef80(self, 500, IP(g_eng.data(0x62e3c0)), IP(g_eng.data(0x62e558)), 78, 200, 0);
            self->base.fe8 = 0;
            self->clip_stand = g_eng.data(0x62a8c8);
            self->f15a80 = 1;
        }
    }

tail:
    if (self->leash != NULL) {
        wl_char *l = self->leash;
        if (self->leash_check != 0 && (l->base.alive == 0 || !g_eng.h40d960(l, 0, 1))) leash_release(self);
        int32_t t = self->leash_t + 1;
        if (t > 0xffffff) t = 0xffffff;
        self->leash_t = t;
        if (self->leash_max > 0 && t > self->leash_max) leash_release(self);
    }
    g_eng.h487530(self);
    if (stunned || (g_eng.h487dc0(self, IP(bb)), self->base.alive != 0)) {
        g_eng.h4878b0(self);
        int32_t code = self->base.code;
        if (code >= 0xc35a && code <= 0xc35c) {
            if (bb->grounded == 0 || bb->vel_x != 0 || bb->pose != 0 || bb->state != 0 || self->f268 != 0) {
                self->f264 = 0;
            } else {
                if (self->f264 > 1000 && self->party->busy != 0 && g_world->scene_kind != 2) body_action_start(bb, 116, 100);
                if (self->f264 < 5000) self->f264 += 50 * (int32_t)rng_rand() / 0x8000;
            }
        }
        g_eng.h46d180(self);
        g_eng.h54d090(self, IP(bb), 0);
        for (int j = 0; j < 2; j++)
            if (self->fc824[j] != 0 && self->fc82c[j] == 0) {
                int32_t one = 1;
                memcpy((uint8_t *)(intptr_t)self->fc824[j] + 612, &one, 4);
                self->fc824[j] = 0;
            }
    }
}
