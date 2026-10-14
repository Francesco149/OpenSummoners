/* char_behave.c — FUN_0047b990.  See char_behave.h.
 *
 * Transcribed from the decompile, its control flow and field widths checked against the
 * asm (the u16 counters: +0x14228..+0x14236, +0x14248, +0x14278, +0x14288/+0x1428a,
 * +0x14290, +0x14804, +0x1482e..+0x14832, +0xc874/+0xc87a/+0xc87c/+0xc89e; air_jumps vs
 * max_air_jumps compare as u16).  The helpers retail calls with `this` in ecx but types
 * stdcall / fastcall (0x4060d0, 0x47de50, 0x47e7c0, 0x482380, 0x481ac0, 0x483b50) are
 * thiscall at these sites. */
#include "char_behave.h"

#include <stddef.h>
#include <string.h>

#include "actor_update.h"
#include "char_ai.h"
#include "char_integrate.h"
#include "engine_calls.h"
#include "entity_move.h"
#include "npc_ai.h"
#include "rng.h"

#define IP(p) ((intptr_t)(p))

static int32_t rnd(void) { return (int32_t)rng_rand(); }
static int32_t mul(int32_t a, int32_t b) { return (int32_t)((uint32_t)a * (uint32_t)b); }
static int32_t iabs(int32_t v) { return v < 0 ? (int32_t)(0u - (uint32_t)v) : v; }
static int32_t world32(uint32_t off) { int32_t v; memcpy(&v, (const uint8_t *)g_world + off, 4); return v; }
static uint16_t *cooldown(wl_char *c, uint16_t i) { return (uint16_t *)((uint8_t *)&c->cd0 + 0x140u * i); }
static int32_t hp_permille(const wl_party *p) { return 1000 * p->hp / (p->hp_max + p->hp_bonus1 + p->hp_bonus2); }

/* the pause length the re-think / target-moved checks draw: h + h * rand / 0x8000 */
static uint16_t pause_ticks(int32_t h) { return (uint16_t)(h + mul((uint16_t)h, rnd()) / 0x8000); }

/* ── the cutscene random walk (cut_walk == 1) ── */
static int random_walk(wl_char *self, wl_body *b, int32_t gr)
{
    int32_t top = self->f15bac, x0 = b->x;
    int32_t can = body_can_act(b);
    int32_t look = 0, sim3 = 0, rise = 0, fall = 0, crouch = 0, jumpable = 0;
    int32_t ahead = 0, behind = 0;  /* v115 / v117 */
    int run_blocked = 0;
    int32_t slide = self->can_jump != 0 ? g_eng.h47da60(self, IP(b)) : 0;   /* v114 */
    int32_t moved = iabs(b->x - x0);
    if ((can != 0 || b->state == 5) && self->up_assist != 0) look = 1;
    int32_t open_l = g_eng.h47dbb0(self, IP(b), 1600, 3, 0, 1, 0);   /* v124 */
    int32_t open_r = g_eng.h47dbb0(self, IP(b), 1600, 1, 0, 1, 0);   /* v122 */
    int32_t wm = self->walk_max, vx = b->vel_x;
    if (wm / 2 < vx) {
        int32_t d = wm >= vx ? 3200 : 6400;
        run_blocked = g_eng.h47dbb0(self, IP(b), d, b->facing == 1 ? 1 : 3, 0, 1, 0) == 0;
    }
    int32_t mm = self->move_mode, walkable;
    if (mm != 0) {
        if (mm <= 0 || mm > 3) {
            walkable = open_r;
            crouch = 0;   /* retail leaves this local uninitialized here (unreachable: modes 0..3) */
        } else {
            int32_t h = g_eng.h4060d0(self, IP(b), 0);
            walkable = 1;
            slide = 1;
            jumpable = can;
            crouch = 0;
            if (h < top + 3200 || (h < top + 800 && b->vvel > -10000)) rise = 1;
            if (h > top - 3200 || (h > top - 800 && b->vvel < 10000)) fall = 1;
            if (self->float_h > 0 && self->float_h <= h) slide = 0;
            if (g_eng.h47d760(self) != 0 && g_eng.h47de50(self, IP(b), self->nav_drop) == 0) {
                crouch = 0;
                fall = 0;
                rise = 1;
            }
        }
    } else {
        walkable = 1;
        jumpable = !((uint16_t)self->air_jumps >= (uint16_t)self->max_air_jumps && gr == 0) && can != 0;
        crouch = gr != 0 && (can != 0 || b->state == 2) && self->can_crouch != 0;
        if (slide != 0) {
            wl_entity *at = b->attach;
            if (at != NULL && at == self->platform && self->nav_c870 == 10) slide = 0;
        }
    }
    if (gr == 0 && self->f15a24 == 0) look = 0;

    /* ── the 5 candidate directions ── */
    int32_t dirs[5];
    if (moved < 800) {
        dirs[0] = 0; dirs[1] = 1; dirs[2] = 2; dirs[3] = 5; dirs[4] = 6;
    } else if (walkable == 0 || ((self->walk_max >= b->vel_x || moved > 8000) &&
                                 (self->walk_max / 2 >= b->vel_x || moved > 3200) && !run_blocked)) {
        if (b->x <= x0) {
            if (moved <= 9600) { dirs[1] = 6; dirs[0] = 2; } else { dirs[0] = 6; dirs[1] = 2; }
            dirs[2] = 0; dirs[3] = 1; dirs[4] = 5;
        } else {
            if (moved <= 9600) { dirs[0] = 1; dirs[1] = 5; } else { dirs[0] = 5; dirs[1] = 1; }
            dirs[2] = 0; dirs[3] = 2; dirs[4] = 6;
        }
    } else if (b->facing != 3) {
        dirs[0] = 5; dirs[1] = 1; dirs[2] = 0; dirs[3] = 2; dirs[4] = 6;
    } else {
        dirs[0] = 6; dirs[1] = 2; dirs[2] = 0; dirs[3] = 1; dirs[4] = 5;
    }

    /* ── the ordered action list ── */
    int32_t acts[12];
    int n = 0;
    if (self->nav_chase == 1 && self->target != NULL && self->nav_skill != 0 &&
        ((b->x > x0 + 8000 && b->facing == 1) || (b->x < x0 - 8000 && b->facing == 3)))
        acts[n++] = 5;
    if (slide != 0 && rise != 0) acts[n++] = 4;
    if (fall != 0) acts[n++] = 2;
    acts[n++] = 1;
    if (look != 0 && b->state == 5) acts[n++] = 3;
    if (crouch != 0) acts[n++] = 2;
    if (look != 0) {
        int32_t ch = self->nav_jump_chance;
        if (hp_permille(self->party) < 300) ch /= 2;
        if (ch > mul(rnd(), 1000) / 0x8000) acts[n++] = 3;
    }
    if (self->nav_skill != 0) acts[n++] = 5;
    if (self->nav_skill2 != 0) {
        int32_t lo = self->nav_range_lo, hi = self->nav_range_hi, d = b->facing != 1 ? b->x - x0 : x0 - b->x;
        if (lo == hi || (d >= lo && d <= hi)) acts[n++] = 6;
    }
    if (slide != 0) acts[n++] = 4;

    /* ── try them ── */
    wl_cmd cmd;
    int r = 0;
    for (uint16_t k = 0; k < 5u; k++) {
        int32_t d = dirs[k];
        int32_t fast = 0, flag;
        if (d == 1 || d == 5) {
            fast = d == 5;
            if (open_l == 0) continue;
            flag = 1;
            if (b->facing == 3) { ahead = 1; behind = 0; } else { ahead = 0; behind = 1; }
        } else if (d == 2 || d == 6) {
            fast = d == 6;
            if (open_r == 0) continue;
            flag = 1;
            if (b->facing == 1) { ahead = 1; behind = 0; } else { ahead = 0; behind = 1; }
        } else {
            flag = 0;
            ahead = 0;
            behind = 0;
        }
        if (fast != 0 && self->can_run == 0) continue;
        for (int a = 0; a < n; a++) {
            switch (acts[a]) {
            case 1:
                cmd_clear(&cmd);
                cmd.lr = d;
                r = g_eng.h47d790(self, IP(&cmd), 32, flag);
                if (r != 0) goto accept;
                break;
            case 2:
                if (gr != 0 && d != 0 && fall == 0) break;
                cmd_clear(&cmd);
                cmd.ud = 10;
                cmd.lr = d;
                r = g_eng.h47d790(self, IP(&cmd), 32, 0);
                if (r != 0) goto accept;
                break;
            case 3:
                if (sim3 != 0) break;
                sim3 = 1;
                cmd_clear(&cmd);
                cmd.ud = 11;
                r = npc_try_cmd(self, &cmd, 32, 0, 0, 0, 1);
                if (r != 0) goto accept;
                break;
            case 4: {
                if (self->air_max != 0 && fast != 0) break;
                cmd_clear(&cmd);
                int32_t j;
                if (b->state <= 1u) {
                    if (jumpable == 0) break;
                    j = 7;
                } else {
                    if (b->state != 3) break;
                    j = 8;
                }
                cmd.lr = d;
                cmd.jump = j;
                r = g_eng.h47d790(self, IP(&cmd), 32, 0);
                if (r != 0) goto accept;
                if (flag != 0) {
                    cmd_clear(&cmd);
                    cmd.jump = j;
                    r = npc_try_cmd(self, &cmd, 32, 0, 0, 0, 1);
                    if (r != 0) goto accept;
                }
                break;
            }
            case 5:
                if (behind == 0) break;
                if (g_eng.h47e010(self, (uint16_t)self->nav_skill, self->nav_skill_arg, IP(b), gr) == 0) break;
                cmd_clear(&cmd);
                cmd_set_skill_kind(&cmd, (int16_t)self->nav_skill, 210, (int16_t)self->nav_skill_arg, 0);
                r = g_eng.h47d790(self, IP(&cmd), 40, 1);
                if (r != 0) goto accept;
                break;
            case 6:
                if (ahead == 0 || fast == 0 ||
                    g_eng.h47e010(self, (uint16_t)self->nav_skill2, self->nav_skill2_arg, IP(b), gr) == 0)
                    break;
                cmd_clear(&cmd);
                cmd_set_skill_kind(&cmd, (int16_t)self->nav_skill2, 210, (int16_t)self->nav_skill2_arg, 0);
                r = g_eng.h47d790(self, IP(&cmd), 40, 1);
                if (r != 0) goto accept;
                break;
            default:
                break;
            }
        }
    }
    if (look == 0 || sim3 != 0) {
        cmd_clear(&cmd);
        r = 0;
    } else {
        cmd_clear(&cmd);
        cmd.ud = 11;
        r = npc_try_cmd(self, &cmd, 32, 0, 0, 0, 1);
        if (r == 0) cmd_clear(&cmd);
    }
accept:
    memcpy(&self->cmd, &cmd, sizeof cmd);
    return r;
}

/* the side of the leader to keep to: ally strength on each side (re-picked every 60 ticks);
 * 1 when it re-picked */
static int pick_side(wl_char *self)
{
    int32_t cur = self->side;
    wl_char *lead = self->leader;
    if (self->side_t != 0) return 0;
    int32_t right = 0, left = 0;
    for (int i = 0; i < 32; i++) {
        wl_char *c = g_world->chars[i];
        if (g_eng.h40d960(c, 0, 0) == 0 || faction_relation(self->base.faction, c->base.faction) != 1) continue;
        int32_t lx = g_eng.h44e640(lead, 0);
        int32_t d = g_eng.h44e640(c, 0) - lx, ad = iabs(d);
        if (ad >= 40000) continue;
        int32_t ly = g_eng.h44e680(lead, 0);
        if (iabs(g_eng.h44e680(c, 0) - ly) >= 20000) continue;
        int32_t sum, w;
        {
            int32_t a, b2, c2;
            memcpy(&a, (const uint8_t *)c->party + 0xe4, 4);
            memcpy(&b2, (const uint8_t *)c->party + 0xe0, 4);
            memcpy(&c2, (const uint8_t *)c->party + 0xd8, 4);
            sum = a + b2 + c2;
        }
        if (g_eng.h485f70(c) != 0) w = 100 * sum;
        else if (ad > 24000) w = 500 * sum;
        else if (ad > 16000) w = 750 * sum;
        else w = 1000 * sum;
        if (d > 0) right += w;
        else left += w;
    }
    int32_t s;
    if (right != 0) {
        if (left != 0) {
            if (cur == 1) right += 1000;
            else if (cur == 3) left += 1000;
            if (self->leader->fc77c == 1) right += 500;
            else left += 500;
            s = left >= right ? 3 : 1;
        } else {
            s = 1;
        }
    } else if (left != 0) {
        s = 3;
    } else {
        s = self->leader->fc77c;
    }
    self->side = s;
    self->side_t = 60;
    return 1;
}

int char_behave(wl_char *self, int32_t cut, wl_body *b, int32_t gr)
{
    int32_t flag = 0;   /* v123: mode 4 */
    if (cut != 0) {
        if (self->cut_walk == 1) return random_walk(self, b, gr);
        if (self->fc850 == 0) return cut;
    }

    /* ── the countdowns ── */
    if (self->f14832 != 0) self->f14832--;
    if (self->f14830 > 1u) {
        if (g_world->f2770 != 0 || g_world->f2764 == 1 || world32(0x2774) != 0) self->f14830--;
        else self->f14830 = 1;
    }
    if (self->f1482e > 1u) self->f1482e--;
    for (uint16_t i = 0; i < self->ncd; i++) {
        uint16_t *cd = cooldown(self, i);
        if (*cd != 0) (*cd)--;
    }
    if (self->f14804 != 0) self->f14804--;
    if (self->side_t != 0) self->side_t--;
    int32_t mode = self->base.coll_off != 0 ? 9 : self->ai_mode;
    /* retail's eax from here to the think countdown below, which only replaces its low
     * word: the countdown's return value carries this upper half */
    uint32_t eax = (uint32_t)(mode - 11);
    if (mode == 3 || mode == 4 || mode == 11) {
        if (mode == 4) flag = 1;
        eax = 0;
        if (pick_side(self)) eax = (uint32_t)self->side;
    }
    if (self->engage_t != 0) self->engage_t--;
    if (self->f14234 != 0) self->f14234--;
    if (self->think_t != 0) {
        self->think_t--;
        return (int)((eax & 0xffff0000u) | self->think_t);
    }
    self->think_t = 1;

    /* ── the re-think pause ── */
    if (self->f14236 != 0) {
        self->f14236--;
    } else {
        if (self->base.coll_off != 0) {
            if (mul(rnd(), 1000) / 0x8000 < 800) self->f14234 = pause_ticks(mul(80, self->pause_len) / 1000 / 2);
        } else if ((int32_t)self->wait_chance > mul(rnd(), 1000) / 0x8000) {
            self->f14234 = pause_ticks(mul(40, self->pause_len) / 1000 / 2);
        }
        self->f14236 = 80;
    }

    /* ── target / ally tracking ── */
    int32_t fac = self->base.faction, g65 = gr;
    if (self->party->busy != 0 || g_world->scene_kind == 2) {
        self->target = NULL;
        self->pal = NULL;
        self->f14288 = 0;
    } else {
        if (self->f14288 == 1) {
            if (self->target != NULL && g_eng.h485f30(self->target) > 0) { self->target = NULL; self->target_t = 0; }
            if (self->target != NULL && g_eng.h40d960(self->target, 0, 0) == 0) { self->target = NULL; self->target_t = 0; }
        }
        if (self->pal != NULL && g_eng.h40d960(self->pal, 0, 0) == 0) { self->pal = NULL; self->target_t = 0; }
        int skip_pick = 0;
        if (self->target_t != 0) {
            self->target_t--;
        } else {
            self->target_t = 16;
            if (fac != 0) {
                self->pal = (wl_char *)(intptr_t)g_eng.h47e330(self, fac, 1, 0, 0, 0, 0, 1);
                if (self->f14288 == 2) skip_pick = 1;
                else g_eng.h4852f0(self, g_eng.h47e330(self, fac, 1, 24, self->reach, 1, flag == 0, 1));
            }
        }
        if (skip_pick || self->f14288 == 2) {
            int32_t need = 0, have = 0;
            if (self->base.coll_off != 0 || self->target == NULL || g_eng.h40d960(self->target, 1, 1) == 0 ||
                g_eng.h47eae0(self, IP(self->f14298), IP(&need), 0, 1) == 0 ||
                g_eng.target_ok_47adb0(self->target, self, *(int32_t *)self->f14298, IP(&have), 0, 0, 1, 0) == 0 ||
                need > have) {
                self->target = NULL;
                self->f14288 = 0;
                g_eng.h484bf0(self, 0);
            }
        } else if (self->engage_t == 0 && self->base.coll_off == 0 && fac != 0) {
            g_eng.h484ea0(self, IP(b), gr);
        }
        if (self->f14288 != 2) {
            if (self->fc794 != 0 && (world_scene_running() || world32(0x2774) != 0)) {
                self->f14288 = 1;
            } else if (self->pal != NULL) {
                int32_t px = g_eng.h44e640(self->pal, 0);
                int32_t dx = iabs(g_eng.h44e640(self, 0) - px);
                int32_t sy = g_eng.h44e680(self, 0), py = g_eng.h44e680(self->pal, 0);
                int32_t dy = iabs(sy - py);
                if (self->f14288 != 0) {
                    if (self->f14288 == 1 && dx > 88000) self->f14288 = 0;
                } else if (dx < 40000 && dy < 20000) {
                    self->f14288 = 1;
                }
                g65 = gr;
            } else if (self->f14288 == 1) {
                self->f14288 = 0;
            }
        }
    }
    /* the target moved: maybe pause */
    if (self->target != NULL && self->pause_chance != 0) {
        const wl_body *tb = self->target->base.body;
        if (iabs(self->target_x - tb->x) >= 800 && iabs(self->target_y - tb->margin - tb->y) >= 800 &&
            (int32_t)self->pause_chance > mul(rnd(), 1000) / 0x8000)
            self->f14234 = pause_ticks((int32_t)((uint32_t)(60 * self->pause_len) / 1000u) / 2);
        tb = self->target->base.body;
        self->target_x = tb->x;
        self->target_y = tb->y + tb->margin;
    }

    /* ── the attack choice while acting ── */
    wl_body *bb = self->base.body;
    if (!(bb->pose == 0 || bb->act_len <= 200u || (self->ai_state >= 5u && self->ai_state <= 6u))) {
        int32_t hold = 0, v119 = 0, v116 = 0;
        if (self->engage_t == 0) {
            if (g_eng.h47f9d0(self, 0x40, 0) == 0) {
                self->engage_t = self->retarget;
            } else {
                hold = 1;
                if (self->pal != NULL && g_eng.h485f70(self->pal) != 0) {
                    int32_t ex = 0, ey = 0;
                    g_eng.h47e7c0(self, IP(self->pal), 0, IP(b), 0, 0, 0, IP(&ex), IP(&ey));
                    if (iabs(ex) < 9600 && iabs(ey) < 9600) {
                        hold = 0;
                        self->engage_t = self->retarget;
                    }
                }
            }
        }
        int32_t *rec = NULL;
        if (self->f14288 == 1) {
            rec = (int32_t *)(intptr_t)g_eng.h47ecd0(self, IP(self->target), 1, IP(b), g65, hold, 1, 0, IP(&v119), IP(&v116));
        } else if (self->f14288 == 2) {
            uint16_t sk;
            memcpy(&sk, *(void **)self->f14298, 2);
            rec = (int32_t *)(intptr_t)g_eng.h47ecd0(self, IP(self->target), 1, IP(b), g65, hold, 0, sk, IP(&v119), IP(&v116));
            if (rec == NULL) self->f14288 = 0;
        }
        if (rec == NULL) {
            cmd_clear(&self->cmd);
            goto done;
        }
        if (rec[3] == 3) {
            g_eng.h484bf0(self, 6);
            self->f14818 = (int32_t)(intptr_t)rec;
            self->f1481c = v119;
            self->f14820 = v116;
        } else {
            g_eng.h484bf0(self, 3);
            self->f14818 = (int32_t)(intptr_t)rec;
            self->f1481c = v119;
        }
    }

    /* ── per-code extras ── */
    {
        int32_t was = self->f14294;
        self->f14294 = 0;
        if (self->f14288 == 0 && was != 0) {
            self->f14288 = 1;
            self->target = NULL;
        }
        int32_t code = self->base.code;
        if (code == 58010) {
            /* the pacer: walks between its two posts */
            self->nav_drop = mul(rnd(), 3200) / 0x8000 + 1600;
            if (self->side_t != 0) self->side_t--;
            int coin = mul(rnd(), 1000) / 0x8000 < 100;
            int32_t post;
            if (self->side == 1) {
                post = self->f14264;
                if (self->side_t == 0 &&
                    !(iabs(b->x - post) > 3200 && g_eng.h47dbb0(self, IP(b), 1600, 1, 1, 0, 0) != 0 && !coin)) {
                    self->side = 3;
                    self->side_t = 60;
                }
            } else {
                post = self->f14268;
                if (self->side_t == 0 &&
                    !(iabs(b->x - post) > 3200 && g_eng.h47dbb0(self, IP(b), 1600, 3, 1, 0, 0) != 0 && !coin)) {
                    self->side = 1;
                    self->side_t = 60;
                }
            }
            npc_navigate(self, post, 3200, b, gr, 0, 0, 0, 1, 0);
            goto done;
        }
        int leash_chk = 0;
        if (code >= 51081 && code <= 51083) {
            wl_char *ls = self->leash;
            if (ls != NULL && (ls->base.alive == 0 || g_eng.h40d960(ls, 0, 1) == 0)) {
                cmd_set_skill_kind(&self->cmd, 45, 800, 0, 0);
                goto done;
            }
            leash_chk = 1;
        } else if (code == 51027 || (code >= 0x18745 && code <= 0x18748) || (code >= 0xc834 && code <= 0xc835)) {
            leash_chk = 1;
        } else if (code > 0xc753 && code <= 0xc78b) {
            if (code >= 0xc77f && code <= 0xc780) {
                const wl_party *p = self->party;
                if (p->hp >= 350 * (p->hp_max + p->hp_bonus1 + p->hp_bonus2) / 1000) {
                    self->reach = 0;
                    self->fc838 = 1000;
                } else {
                    self->reach = 22400;
                    self->fc838 = 3200;
                }
            }
        } else if (code == 50010) {
            uint16_t st = self->ai_state;
            if (st == 0 || st == 2) {
                if (self->party->member == 1) {
                    if (self->f14288 != 0) {
                        self->calm_t = 0;
                    } else if (++self->calm_t > 200u || g_world->scene_kind == 2) {
                        cmd_set_skill_kind(&self->cmd, 4, 210, 0, 0);
                        self->calm_t = 0;
                        goto done;
                    }
                } else if (self->f14288 == 1) {
                    self->calm_t = 0;
                    if (g_eng.h47f670(self, 0x1e, 0) != 0) {
                        cmd_set_skill_kind(&self->cmd, 3, 210, 0, 0);
                        goto done;
                    }
                }
            }
        }
        if (leash_chk && g_eng.h485350(self, 3, 4, 20000, 200000, was) != 0) goto done;
    }

    /* ── the leash ── */
    {
        wl_char *ls = self->leash;
        if (ls != NULL && self->fall_warp != 0 && ls->base.alive != 0 && g_eng.h40d960(ls, 0, 1) != 0) {
            if (g_eng.h47b7c0(self, 48000, 24000, IP(ls), 0) != 0) {
                self->leash_lost = 0;
            } else if (++self->leash_lost > 20) {
                cmd_set_skill_kind(&self->cmd, (int16_t)self->fall_warp, 800, 0, 0);
                self->leash_lost = 0;
                goto done;
            }
        }
    }

    /* ── the behaviour state machine ── */
    if (g_eng.h484c10(self, IP(b), g65) == 0) {
        if (self->f14270 != 0 && self->ai_state != 0) {
            self->f14274 = 1000;
            self->f14270 = 0;
        }
        switch (self->ai_state) {
        case 2:
            if (g_eng.h482ac0(self, IP(b)) != 0) break;
            if (self->f1423c > 0) {
                self->f1423c--;
                g_eng.h482880(self, IP(b), g65, IP(self->target));
                break;
            }
            if (g_eng.h4834f0(self, g65, IP(b)) == 0 && g_eng.h481310(self, IP(b), g65, 0) == 0)
                g_eng.h482880(self, IP(b), g65, IP(self->target));
            break;
        case 3:
            cmd_clear(&self->cmd);
            if (self->target == NULL) { g_eng.h484bf0(self, 0); break; }
            if (g_eng.h482380(self, IP(self->target), 0, IP(self->base.body), IP(&self->cmd), self->f14818,
                              IP(&self->ai_sub), self->f1481c) != 0) {
                g_eng.h482240(self, self->f14818);
                if (g_eng.h482460(self, 1) == 0) g_eng.h484bf0(self, 0);
            }
            break;
        case 4:
            self->f14234 = 0;
            g_eng.h482e10(self, IP(b), g65);
            break;
        case 5:
            cmd_clear(&self->cmd);
            if (self->target == NULL) { g_eng.h484bf0(self, 0); break; }
            if (g_eng.h483b50(self, IP(self->target), 0, IP(self->base.body), IP(&self->cmd), IP(&self->ai_sub),
                              self->f14808) != 0 &&
                g_eng.h482460(self, 1) == 0)
                g_eng.h484bf0(self, 0);
            if (self->f1480c == 0 && self->ai_sub >= 3u && g_eng.h481310(self, IP(b), gr, 1) != 0) self->f1480c = 1;
            break;
        case 6:
            g_eng.h481ac0(self, IP(b), g65);
            break;
        default: {
            int32_t m = self->base.coll_off != 0 ? 9 : self->ai_mode;
            switch (m) {
            case 1: {
                int32_t mm = self->move_mode;
                if (mm != 0) {
                    if (mm > 0 && mm <= 3) cmd_clear(&self->cmd);
                    break;
                }
                if (g65 != 0 && self->f14258 == 0) {
                    self->f1425c = b->x;
                    self->f14260 = b->y;
                    self->f14258 = 1;
                }
                if (self->f14258 != 0 &&
                    g_eng.h484530(self, IP(b), g65, self->f1425c, self->f14260, 99, 1, 0) != 0)
                    g_eng.h481310(self, IP(b), g65, 0);
                break;
            }
            case 2:
                if (g_eng.h485990(self, IP(b), g65) == 0) {
                    npc_navigate(self, b->x, 800, b, g65, 0, 0, 0, 0, 0);
                    int32_t w = world32(0x27ac);
                    if (w != 0) {
                        int32_t wx, wy;
                        memcpy(&wx, (const uint8_t *)(intptr_t)w + 108, 4);
                        memcpy(&wy, (const uint8_t *)(intptr_t)w + 112, 4);
                        g_eng.h483de0(self, IP(b), g65, wx, wy, 1);
                    }
                }
                break;
            case 3: case 4: case 11:
                g_eng.h483fb0(self, IP(b), g65, flag, cut);
                break;
            case 5: case 6:
                if (self->base.faction == 0) { cmd_clear(&self->cmd); break; }
                if (g_eng.h485990(self, IP(b), g65) == 0 && g_eng.h485670(self, IP(b), g65, 20) == 0)
                    g_eng.h4857f0(self, IP(b), g65, 20, 0);
                break;
            case 8:
                if (g_eng.h485990(self, IP(b), g65) == 0 && g_eng.h485670(self, IP(b), g65, 10) == 0)
                    g_eng.h4856d0(self, IP(b), g65);
                break;
            case 9:
                if (g_eng.h485990(self, IP(b), g65) == 0) npc_navigate(self, b->x, 800, b, g65, 0, 0, 0, 0, 0);
                break;
            case 10:
                if ((self->party->member != 1 || g_eng.h485990(self, IP(b), g65) == 0) &&
                    g_eng.h485670(self, IP(b), g65, 10) == 0)
                    g_eng.h4856d0(self, IP(b), g65);
                break;
            case 12:
                if (self->ai_sub != 0) {
                    if (self->ai_sub != 1) {
                        if (g_eng.h4857f0(self, IP(b), g65, 20, 10) != 0) {
                            int32_t r = rnd();
                            self->ai_sub = 1;
                            self->ai_t = (uint16_t)(mul(r, 100) / 0x8000 + 50);
                        }
                        break;
                    }
                } else {
                    int32_t r = rnd();
                    self->ai_sub++;
                    self->ai_t = (uint16_t)(mul(r, 100) / 0x8000);
                }
                cmd_clear(&self->cmd);
                if (self->ai_t != 0) self->ai_t--;
                else self->ai_sub++;
                break;
            case 13:
                cmd_clear(&self->cmd);
                break;
            case 14:
                cmd_clear(&self->cmd);
                g_eng.h485740(self, IP(b), g65, 10);
                break;
            default:
                break;
            }
            break;
        }
        }
    }

done:
    /* ── the leader-follow run boost ── */
    {
        wl_char *lead = self->leader;
        self->boost_speed = 0;
        if (lead == NULL) return 0;
        if (g_eng.h4854e0(self, IP(b), gr) != 0) self->boost_speed = 1;
        if (self->boost_speed != 0) {
            self->boost_accel = 10;
            return 1;
        }
        if (self->boost_accel > 0) self->boost_accel--;
        return self->boost_accel;
    }
}
