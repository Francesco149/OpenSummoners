/* char_integrate.c — FUN_00442a70 and its small helpers.  See char_integrate.h.
 *
 * Transcribed from the decompile; call arities checked against each callee's retn /
 * the call-site pushes (0x456d30 and 0x467260 are thiscall — IDA types them fastcall
 * with a phantom edx argument).  Labels keep retail's control flow. */
#include "char_integrate.h"

#include <stddef.h>
#include <string.h>

#include "arche_pose.h"
#include "char_physics.h"
#include "char_state.h"
#include "char_handlers.h"
#include "engine_calls.h"
#include "particle_spawn.h"
#include "entity_life.h"
#include "entity_move.h"
#include "map_grid.h"
#include "rng.h"
#include "arche_sword.h"
#include "sana_skills.h"
#include "skill_cast.h"
#include "sound_fx.h"

long g_integrate_unported;

#define IP(p) ((intptr_t)(p))

/* ── helpers ──────────────────────────────────────────────────────────────── */

int body_action_start(wl_body *b, int16_t pose, int32_t len)
{
    if (!(b->act_len / 100 <= len / 100 || b->pose == 0)) return 0;
    if ((uint16_t)pose == b->pose) return 0;
    b->act_len = (uint16_t)len; b->f58 = 0; b->act_ctr = 0; b->act_78 = 0; b->act_arg = 0;
    b->spell = 0; b->target = 0; b->act_5a = 0; b->act_64 = 0;
    b->pose = (uint16_t)pose;
    return 1;
}

void body_action_reset(wl_body *b)
{
    b->pose = 0; b->act_len = 210; b->f58 = 0; b->act_ctr = 0; b->act_78 = 0;
    b->act_arg = 0; b->spell = 0; b->target = 0; b->act_5a = 0; b->act_64 = 0;
}

void body_set_state(wl_body *b, int16_t st) { b->state = (uint16_t)st; b->sub = 0; b->ctr = 0; }

void body_set_clip(wl_body *b, const void *clip)
{
    if (b->clip == clip) return;
    b->clip = clip; b->clip_done = 0; b->clip_frame = 0; b->clip_timer = 0;
}

static uint16_t clip_u16(const void *c, size_t off) { uint16_t v; memcpy(&v, (const uint8_t *)c + off, 2); return v; }
static int32_t  clip_i32(const void *c, size_t off) { int32_t v; memcpy(&v, (const uint8_t *)c + off, 4); return v; }

void body_clip_step(wl_body *b)
{
    const void *c = b->clip;
    if (c == NULL || ++b->clip_timer < clip_u16(c, 0x44)) return;
    uint16_t f = ++b->clip_frame;
    b->clip_timer = 0;
    if (f < clip_u16(c, 0x42)) return;
    if (clip_i32(c, 0x48) != 0) {
        b->clip_timer = 1; b->clip_done = 1; b->clip_frame = (uint16_t)(clip_u16(c, 0x42) - 1);
    } else {
        b->clip_frame = clip_u16(c, 0x152);
    }
}

static int32_t grid_i32(size_t off) { int32_t v; memcpy(&v, g_world->grid + off, 4); return v; }

/* region C +0 at (cell of the body's centre x, row of `wy`): 1 = a half cell (the lower half counts) */
static int32_t region_c_flag(int32_t cx, int32_t wy)
{
    int32_t col = cx / 3200, row = wy / 3200;
    if (col < 0) col = 0;
    if (grid_i32(MG_DIM0) - 1 < col) col = grid_i32(MG_DIM0) - 1;
    if (row < 0) row = 0;
    if (grid_i32(MG_DIM1) - 1 < row) row = grid_i32(MG_DIM1) - 1;
    return grid_i32(MG_REGION_C + 12u * (uint32_t)((col << 7) + row));
}

int body_region_flags(wl_body *b)
{
    int32_t bottom = b->h + b->y - 1, cx = b->w / 2 + b->x;
    int32_t f = region_c_flag(cx, bottom);
    b->c_flag = f == 1 ? bottom % 3200 > 1600 : f;
    int32_t top = b->y + b->margin;
    f = region_c_flag(cx, top);
    if (f == 1) { b->slow = top % 3200 > 1600; return b->slow; }
    b->slow = f;
    return f - 1;
}

void char_copy_rects(const wl_char *self, wl_body *b)
{
    for (uint16_t i = 0; i < self->base.nrects; i++) {
        uint8_t *dst = (uint8_t *)b->frames + 20u * i;
        memcpy(dst, self->base.rects + 32u * i, 16);
        memset(dst + 16, 0, 4);
    }
}

int skill_pose_ok(const wl_skill *k, const wl_body *b)
{
    int32_t req = k->pose_req;
    if (req != 0) {
        uint16_t lvl = b->act_64;
        if (lvl >= k->pose_lo && lvl <= k->pose_hi) {
            if (b->pose == (uint16_t)req) return 1;
            if (req == 10101) {
                int32_t d = (int32_t)b->pose - 10115;
                if (d != 0 ? d == 7 : b->act_arg == 0) return 1;
            }
        }
    } else if (b->pose == 0 || b->act_len <= 200u) {
        return 1;
    }
    return 0;
}

int char_skill_ok(wl_char *self, const wl_skill *k, int32_t probe)
{
    if (k == NULL) return 0;
    switch (k->cls) {
    case 0xf00000:
        break;
    case 0xf00001:
        if (self->party->f466 != 0) return 0;
        if (party_find(self, 50013, 0, NULL, 0) == NULL) return 0;
        break;
    case 0xf00003:
        if (self->party->member != 1) return 0;
        /* fall through */
    case 0xf00002:
        if (self->party->f466 == 0) return 0;
        break;
    default:
        if (k->cls != self->party->member) return 0;
        break;
    }
    switch (k->mode) {
    case 1: case 2: return 1;
    case 3:         return probe == 0;
    case 0xff:      return self->base.coll_off != 0;
    default:        return 0;
    }
}

int32_t body_facing_sign(const wl_body *b, int32_t v) { return b->facing == 3 ? -v : v; }

int body_special_clip(wl_body *b, int32_t probe)
{
    (void)probe;
    if (b->pose == 31) {
        if (b->f58 == 0) { body_set_clip(b, g_eng.data(0x6298d8)); b->f58 = 1; }
    } else if (b->pose == 123) {
        if (b->f58 == 0) { body_set_clip(b, g_eng.data(0x629740)); b->f58 = 1; }
        return 1;
    }
    return 0;
}

int body_can_act(const wl_body *b)
{
    switch (b->state) {
    case 0: case 1: case 4: return 1;
    case 2:                 return b->sub == 2;
    default:                return 0;
    }
}

int char_carry(wl_char *self, wl_char *other)
{
    if (self->can_carry == 0) return 0;
    if (other != NULL) {
        if (other->base.alive != 1 || other->base.body->active != 1 || other->party->busy != 0) return 0;
        if (other->carrier != NULL || self->held != NULL) return 0;
        other->carrier = self;
        self->held = other;
        return 1;
    }
    if (self->held != NULL) { self->held->carrier = NULL; self->held = NULL; }
    return 1;
}

int char_float_cap(wl_char *self, wl_body *b, int32_t part, int32_t keep_speed)
{
    int32_t mm = self->move_mode;
    if (mm > 0) {
        if (mm <= 2) goto cap;
        if (mm == 3) {
            int32_t y = b->y;
            int blocked = entity_move_vertical(&self->base, b, 1, b->drop, (uint16_t)part, &y,
                                               (wl_entity **)&b->attach) == 0;
            if (b->attach != NULL) b->y = y;
            if (blocked) {
                if (keep_speed == 0) b->vel_x = 0;
                b->vvel = 0;
                return keep_speed;
            }
            goto cap;
        }
    }
    if (self->fall_cap > 56000) return mm;
cap:;
    int32_t c = self->fall_cap;
    if (c <= 64000) c = 64000;
    b->fall_cap = c;
    return mm;
}

/* ── FUN_00442a70 ─────────────────────────────────────────────────────────── */

static int special_pose(uint16_t p) { return (p >= 0x75 && p <= 0x7a) || p == 0x81; }

int char_integrate(wl_char *self, wl_cmd *cmd, wl_body *in, wl_body *out, int32_t probe, int32_t part)
{
    if (special_pose(in->pose))        /* PORT-DEBT(integrate-special-poses) */
        return g_eng.integrate_442a70(self, cmd, in, out, probe, part);

    int32_t ctl = 0;        /* v220 */
    int32_t dt = -20;       /* v221: the +0x66 timer step the handlers may change */
    int32_t held = 0;       /* v224 */
    wl_skill *rec = NULL;   /* a3 */

    /* ── in -> out (the fields NOT copied keep out's own bytes: +0x68, +0x82..) ── */
    out->active = in->active;
    out->x = in->x; out->y = in->y; out->w = in->w; out->h = in->h;
    out->margin = 0;
    out->vvel = in->vvel;
    out->fall_cap = 0;
    out->drop = in->drop;
    out->grounded = in->grounded; out->vel_x = in->vel_x; out->facing = in->facing;
    memcpy((uint8_t *)out + 0x30, (const uint8_t *)in + 0x30, 8);     /* +0x30, +0x34 */
    out->state = in->state; out->sub = in->sub; out->ctr = in->ctr;
    out->dst_x = self->base.dst_x; out->dst_y = self->base.dst_y;
    out->pose = in->pose; out->spell = in->spell; out->target = in->target;
    out->act_arg = in->act_arg; out->act_len = in->act_len; out->f58 = in->f58; out->act_5a = in->act_5a;
    out->act_ctr = in->act_ctr; out->serial = in->serial; out->act_64 = in->act_64; out->f66 = in->f66;
    out->clip = in->clip; out->clip_timer = in->clip_timer; out->clip_frame = in->clip_frame;
    out->clip_done = in->clip_done; out->act_78 = in->act_78;
    {
        int32_t nc; memcpy(&nc, (const uint8_t *)self + 0x268, 4);
        out->nocollide = nc != 0; out->f7d = nc != 0;
    }
    out->f7e = 0; out->f7f = 0; out->nhit = 0;
    out->attach = in->attach; out->slow = in->slow; out->c_flag = in->c_flag;
    if (self->base.rects != NULL) char_copy_rects(self, out);

    /* ── the ground probe ── */
    int32_t mm = self->move_mode;
    if (mm >= 0) {
        if (mm <= 2) {
            int32_t y = out->y;
            int clear = entity_move_vertical(&self->base, out, 1, out->drop, (uint16_t)part, &y,
                                             (wl_entity **)&out->attach);
            if (out->attach != NULL) out->y = y;
            out->grounded = clear == 0;
        } else if (mm == 3) {
            out->grounded = 0;
        }
    }
    int was_idle = out->pose == 0;
    out->w = self->base.width; out->h = self->base.height;
    if (!was_idle && out->act_len / 100 <= 1 &&
        (cmd->skill != 0 || cmd->jump != 0 || cmd->lr != 0 || cmd->ud != 0 || cmd->f04 != 0))
        body_action_reset(out);
    if (self->party->busy != 0) out->nocollide = 1;

    /* ── start the commanded action ── */
    if (self->party->busy != 0 && cmd->skill_kind != 900) goto actions;
    if (cmd->skill != 0) {
        int32_t i = 0, n = self->n_skills;
        if (n != 0) {
            while (self->skills[i].id != (int32_t)cmd->skill)
                if (++i >= n) goto no_record;
            rec = &self->skills[i];
            int32_t gr = out->grounded;
            if (char_skill_ok(self, rec, 0) == 0 || skill_pose_ok(rec, out) == 0) goto actions;
            switch (rec->ground) {
            case 1: if (gr == 0) goto actions; break;
            case 2: if (gr != 0) goto actions; break;
            case 3: if (gr == 0 || out->vel_x < 900 * self->max_speed / 1000) goto actions; break;
            default: break;
            }
            goto start;
        }
no_record:
        rec = NULL;
        if (out->pose != 0 && out->act_len > 200u) goto actions;
start:;
        int32_t sp = cmd->skill_ptr;
        uint16_t sa = cmd->skill_arg;
        if (body_action_start(out, (int16_t)cmd->skill, cmd->skill_kind) != 0) {
            out->act_arg = sa;
            out->spell = (int32_t)(intptr_t)rec;
            out->target = sp;
            int32_t s = g_world->action_serial;
            if (s >= 0xffffff) g_world->action_serial = 0;
            else g_world->action_serial = s + 1;
            out->serial = s + 1;
        }
    }

actions:
    switch (out->pose) {
    case 0x20:
        if (probe == 0) {
            int32_t c = out->act_ctr;
            if (c >= 79) {
                body_action_reset(out);
                if (self->f15904 != 0) char_release(self);
                else { body_action_reset(out); body_action_start(out, 122, 800); }
            } else {
                int32_t n = c + 1;
                out->act_ctr = n;
                int32_t t; memcpy(&t, (const uint8_t *)g_eng.sys + 10232 - 4 * (n / 4), 4);
                self->base.tint_f4 = t; self->base.tint_f8 = t;
            }
        }
        goto tail;
    case 0x68: case 0x6a: {
        int32_t c = out->act_ctr;
        ctl = 3;
        if (c == 0) {
            voice_cue(self, probe, 9, 0, 0, 0);
            ih_467260((wl_char *)(intptr_t)(self), (int32_t)(IP(out)), (int32_t)(probe), (int32_t)(self->f158fc));
        }
        body_set_clip(out, out->pose == 106 ? self->clip_hit2 : self->clip_hit);
        c = out->act_ctr;
        out->nocollide = 1;
        out->act_ctr = c + 1;
        if (c + 1 > 20) body_action_reset(out);
        char_float_cap(self, out, part, 1);
        goto dispatch;
    }
    case 0x69:
        ctl = 3;
        if (out->f58 != 0) {
            if (out->f58 != 1) goto hit_tail;
        } else {
            voice_cue(self, probe, 9, 0, 0, 0);
            body_set_clip(out, self->clip_hit);
            out->f58++;
            ih_467260((wl_char *)(intptr_t)(self), (int32_t)(IP(out)), (int32_t)(probe), (int32_t)(self->f158fc));
        }
        if (out->clip_done != 0) body_action_reset(out);
hit_tail:
        out->nocollide = 1;
        char_float_cap(self, out, part, 1);
        goto dispatch;
    default:
        if (special_pose(out->pose)) { g_integrate_unported++; goto tail; }
        goto dispatch;
    }

dispatch:
    body_clip_step(out);
    {
        const int32_t gr = out->grounded;
        uint32_t code = (uint32_t)self->base.code;
        int r;
#define PHYS() char_physics_step(self, cmd, out, probe, ctl, part, out->grounded)
#define ANIM() char_anim_select(self, out, probe, out->grounded)
        if (code > 0xc82a) {
            if (code > 0x18749) {
                switch (code) {
                case 0x1874a: goto t904;
                case 0x1874b: goto t885;
                case 0x1874c: case 0x1874d: goto t874;
                case 0x1874e:
                    if (g_eng.h446900(self, IP(out), probe, IP(&ctl), part, gr, IP(&dt), IP(cmd)) == 0) goto t913;
                    goto t914;
                case 0x1874f: case 0x18750: case 0x18751: goto t881;
                case 0x18752:
                    g_eng.h449810(self, IP(out), probe, IP(&ctl), gr, IP(cmd), part, IP(&dt));
                    goto t900;
                case 0x18753: case 0x18754:
                    if (g_eng.h4539b0(self, IP(out), probe, IP(&ctl), part, gr, IP(&held)) == 0) goto t913;
                    goto t914;
                case 0x18759:
                    if (self->party->member == 1)
                        skill_cast(self, IP(out), probe, IP(&ctl), IP(cmd), gr, &dt, (uint16_t *)g_eng.data(0x648650), 210);
                    if (g_eng.h44d8f0(self, IP(out), probe, IP(&ctl), part, out->grounded, IP(cmd), IP(&held)) == 0) goto t913;
                    goto t914;
                case 0x1875a: goto t890;
                case 0x1875b:
                    {
                        if ((**g_eng.keycfg)->effects != 0) {
                            int32_t dy = 1600 * (int32_t)rng_rand() / 0x8000;
                            int32_t rx = (int32_t)rng_rand();
                            particle_spawn(&self->base, &self->base, 100060, out, 4000 - 8000 * rx / 0x8000, dy, 99, 1, probe, 0, 2, 0, 0, 0);
                        }
                    }
                    g_eng.h448dc0(self, IP(out), probe, IP(&ctl), out->grounded, IP(cmd), part, IP(&dt));
                    goto t909;
                case 0x1875c: case 0x1875d: goto t883;
                default: goto t834;
                }
            }
            if (code != 100169) {
                if (code <= 0xe2a6) {
                    if (code != 58022) {
                        if (code <= 0xe29a) {
                            if (code != 58010) {
                                switch (code) {
                                case 0xc834: case 0xc835: goto t867;
                                case 0xc83d: case 0xc83e: goto t883;
                                default: goto t834;
                                }
                            }
                            PHYS();
                            goto tail;
                        }
                        if (code == 58020) { char_idle_voice(self, probe, 73, 100, 100); goto t834; }
                        if (code != 58021) goto t834;
                    }
                    goto t860;
                }
                if (code <= 0xea59) {
                    if (code == 59993) goto t861;
                    if (code - 58023 > 1) goto t834;
t860:
                    char_idle_voice(self, probe, 73, 400, 100);
                    goto t834;
                }
                if (code == 100164) goto t890;
                if (code <= 0x18744) goto t834;
t867:
                if (self->party->member == 1)
                    skill_cast(self, IP(out), probe, IP(&ctl), IP(cmd), out->grounded, &dt, (uint16_t *)g_eng.data(0x65a028), 210);
                g_eng.h44b160(self, IP(out), probe, IP(&ctl), out->grounded, IP(cmd), part);
                PHYS();
                if (out->pose != 0) goto tail;
                if (self->party->member != 1) goto tail;
                goto t837;
            }
t871:
            skill_cast(self, IP(out), probe, IP(&ctl), IP(cmd), out->grounded, &dt, (uint16_t *)g_eng.data(0x64c418), 210);
            if (g_eng.h4516e0(self, IP(out), probe, IP(&ctl), part, out->grounded, IP(&dt), IP(cmd)) == 0) goto t900;
            goto t901;
        }
        if (code >= 0xc829) {
t904:
            if (g_eng.h4484b0(self, IP(out), probe, IP(&ctl), out->grounded, IP(cmd), part, IP(&dt)) != 0) goto t910;
            goto t909;
        }
        if (code > 0xc763) {
            switch (code) {
            case 0xc76b: case 0xc76c:
t806:
                if (ih_4512d0((wl_char *)(intptr_t)(self), (int32_t)(IP(out)), (int32_t)(probe), (int32_t *)(intptr_t)(IP(&ctl)), (int32_t)(part), (int32_t)(out->grounded), (int32_t)(IP(cmd))) == 0) goto t913;
                goto t914;
            case 0xc775: case 0xc776: goto t814;
            case 0xc77f: case 0xc780:
                g_eng.h44a200(self, IP(out), probe, IP(&ctl), out->grounded, IP(cmd), part);
                goto t900;
            case 0xc789: case 0xc78a: case 0xc78b:
                if (self->party->member == 1)
                    skill_cast(self, IP(out), probe, IP(&ctl), IP(cmd), out->grounded, &dt, (uint16_t *)g_eng.data(0x64ac48), 210);
                if (g_eng.h44e8d0(self, IP(out), probe, IP(&ctl), part, out->grounded, IP(cmd)) == 0) goto t900;
                goto t901;
            case 0xc792: case 0xc793: case 0xc794:
                r = g_eng.h44f690(self, IP(out), probe, IP(&ctl), part, out->grounded, IP(cmd), IP(&dt), IP(&held));
                if (out->state == 5) out->fall_cap = 4000;
                if (r == 0) goto t913;
                goto t914;
            case 0xc79c: case 0xc7a6: case 0xc7a7: case 0xc7b0: case 0xc7b1: case 0xc7b2: goto t874;
            case 0xc80b: case 0xc80c: case 0xc80d:
                g_eng.h448dc0(self, IP(out), probe, IP(&ctl), out->grounded, IP(cmd), part, IP(&dt));
                goto t909;
            case 0xc815:
t812:
                if (g_eng.h450810(self, IP(out), probe, IP(&ctl), part, out->grounded, IP(cmd)) == 0) goto t909;
                goto t910;
            case 0xc81f: case 0xc820: goto t881;
            default: goto t834;
            }
        }
        if (code >= 0xc760) goto t871;
        if (code > 0xc74e) {
            switch (code) {
            case 0xc74f: goto t806;
            case 0xc750: goto t881;
            case 0xc751: goto t812;
            case 0xc753: goto t867;
            case 0xc756: case 0xc757: goto t885;
            default: goto t834;
            }
        }
        if (code == 51022) {
t814:
            if (ih_44d2e0((wl_char *)(intptr_t)(self), (int32_t)(IP(out)), (int32_t)(probe), (int32_t)(IP(&ctl)), (int32_t)(part), (int32_t *)(intptr_t)(out->grounded)) == 0) goto t909;
            goto t910;
        }
        if (code <= 0xc35d) {
            if (code == 50013) {
                if (g_eng.h456d30(self, IP(out), probe, (uint16_t)part, IP(&ctl), out->grounded) == 0) goto t913;
                goto t914;
            }
            if (code == 50010) {
                r = arche_pose(self, out, probe, part, &ctl);
                skill_cast(self, IP(out), probe, IP(&ctl), IP(cmd), out->grounded, &dt, (uint16_t *)g_eng.data(0x62f868), 210);
                if (r == 0 && arche_sword(self, out, probe, &ctl, out->grounded, part, cmd) == 0) PHYS();
            } else if (code == 50011) {
                r = arche_pose(self, out, probe, part, &ctl);
                skill_cast(self, IP(out), probe, IP(&ctl), IP(cmd), out->grounded, &dt, (uint16_t *)g_eng.data(0x636a58), 210);
                sana_skills(self, out, probe, &ctl, cmd, out->grounded);
                if (r == 0) PHYS();
            } else if (code == 50012) {
                goto t861;
            } else {
                goto t834;
            }
            if (out->pose != 0) goto tail;
            goto t795;
        }
        if (code == 50350) {
            if (sys_by_mode(g_eng.sys, 1, 1, 1) != 0) goto t834;
            goto t860;
        }
        if (code > 0xc741 && code <= 0xc747) goto t890;
        goto t834;

t861:
        r = arche_pose(self, out, probe, part, &ctl);
        skill_cast(self, IP(out), probe, IP(&ctl), IP(cmd), out->grounded, &dt, (uint16_t *)g_eng.data(0x63cde8), 210);
        sana_skills(self, out, probe, &ctl, cmd, out->grounded);
        if (r == 0) PHYS();
        if (out->pose != 0) goto tail;
t795:
        ANIM();
        goto tail;
t890:
        if (out->grounded == 0) out->nocollide = 1;
        if (ih_455d80((wl_char *)(intptr_t)(self), (int32_t)(IP(out)), (int32_t)(probe), (int32_t *)(intptr_t)(IP(&ctl)), (int32_t)(part), (int32_t)(out->grounded), (int32_t *)(intptr_t)(IP(&dt)), (int32_t)(IP(cmd))) == 0) PHYS();
        if (out->pose == 0) {
            ANIM();
            if (out->state == 2) out->nocollide = 1;
        }
        goto tail;
t885:
        g_eng.h44a740(self, IP(out), probe, IP(&ctl), out->grounded, IP(cmd), part);
        goto t900;
t874:
        {
            int32_t g = out->grounded;
            if (g == 0) out->nocollide = 1;
            if (skill_cast(self, IP(out), probe, IP(&ctl), IP(cmd), g, &dt, (uint16_t *)g_eng.data(0x66d398), 220) != 0 &&
                probe == 0 && out->f58 == 1)
                voice_cue(self, 0, 10, 1, 0, 0);
            g_eng.h44bc10(self, IP(out), probe, IP(&ctl), out->grounded);
            g_eng.h44c250(self, IP(out), probe, IP(&ctl), out->grounded);
            g_eng.h44bf60(self, IP(out), probe, IP(&ctl), out->grounded);
        }
        goto t900;
t881:
        if (g_eng.h452020(self, IP(out), probe, IP(&ctl), part, out->grounded, IP(&dt), IP(cmd)) == 0) goto t909;
        goto t910;
t883:
        if (g_eng.h452730(self, IP(out), probe, IP(&ctl), part, out->grounded) == 0) goto t909;
        goto t910;
t834:
        if (body_special_clip(out, probe) == 0) PHYS();
        if (out->pose == 0) goto t837;
        goto tail;
t900:
        PHYS();
t901:
        if (out->pose != 0) goto tail;
        goto t837;
t909:
        PHYS();
t910:
        if (out->pose != 0) goto tail;
        goto t837;
t913:
        PHYS();
t914:
        if (out->pose == 0) goto t837;
        goto tail;
t837:
        ANIM();
#undef PHYS
#undef ANIM
    }

tail:
    {
        uint32_t code = (uint32_t)self->base.code;
        if (code >= 0x18753 && code <= 0x18754 && self->base.f08 != NULL) {
            g_eng.h445e50(self, IP(out), 1001, IP(out->frames), IP((uint8_t *)out->frames + 4));
            if (self->base.f08 != NULL)
                g_eng.h445e50(self, IP(out), 1006, IP((uint8_t *)out->frames + 20), IP((uint8_t *)out->frames + 24));
        }
    }
    if (probe == 0 && held == 0 && self->carrier != NULL) char_carry(self->carrier, NULL);
    if (out->f66 != 0) {
        if (out->state == 5) out->f66 = out->f66 < 100u ? 0 : (uint16_t)(out->f66 - 100);
        if (dt != 0) out->f66 = (int32_t)out->f66 + dt < 0 ? 0 : (uint16_t)(out->f66 + dt);
    }
    if (out->f68 != 0 && out->f66 == 0) out->f68 = 0;
    return body_region_flags(out);
}
