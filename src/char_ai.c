/* char_ai.c — FUN_00478ba0 and its small helpers.  See char_ai.h.
 *
 * Transcribed from the decompile with the asm as the tie-breaker (store widths,
 * the unsigned walk-start delay compare, the input-manager offsets IDA indexed
 * through a recovered class).  Labels keep retail's control flow where the
 * decompile has gotos. */
#include "char_ai.h"

#include <stddef.h>
#include <string.h>

#include "char_state.h"
#include "char_integrate.h"
#include "behave_helpers.h"
#include "engine_calls.h"
#include "party_member.h"
#include "input.h"
#include "sound_fx.h"

#define KEYCFG() (**g_eng.keycfg)

void cmd_clear(wl_cmd *c)
{
    c->f04 = 0; c->lr = 0; c->jump = 0; c->ud = 0;
    c->skill = 0; c->act = 0; c->skill_arg = 0; c->skill_ptr = 0; c->skill_kind = 0;
}

void cmd_set_skill(wl_cmd *c, int16_t skill, int16_t arg, int32_t ptr)
{
    c->skill = (uint16_t)skill; c->skill_kind = 210; c->skill_arg = (uint16_t)arg; c->skill_ptr = ptr;
}

void cmd_set_skill_kind(wl_cmd *c, int16_t skill, int16_t kind, int16_t arg, int32_t ptr)
{
    c->skill = (uint16_t)skill; c->skill_kind = (uint16_t)kind; c->skill_arg = (uint16_t)arg; c->skill_ptr = ptr;
}

void cmd_set_member_skill(wl_cmd *c, int16_t skill, int16_t kind)
{
    c->skill = (uint16_t)skill; c->skill_kind = (uint16_t)kind; c->skill_arg = 0; c->skill_ptr = 0;
}

/* The default arm indexes by the low word of the party POINTER (retail bug, quirk #132):
 * reproduced byte-for-byte — in retail memory it reads what retail reads. */
uint16_t party_slot_get(wl_party *p)
{
    uint32_t i;
    switch (p->slot_mode) {
    case 1:  i = 0; break;
    case 2:  i = 1; break;
    case 3:  i = 2; break;
    default: i = (uint32_t)(uintptr_t)p & 0xffffu; break;
    }
    uint16_t v; memcpy(&v, (const uint8_t *)p + 0x138 + 2u * i, 2);
    return v;
}

uint16_t party_slot_set(wl_party *p, int16_t slot)
{
    uint16_t i;
    switch (p->slot_mode) {
    case 1:  i = 0; break;
    case 2:  i = 1; break;
    case 3:  i = 2; break;
    default: i = (uint16_t)slot; break;
    }
    memcpy((uint8_t *)p + 0x138 + 2u * i, &slot, 2);
    return i;
}

int char_ring_double(wl_char *self, uint32_t now, uint32_t lo, uint32_t hi, uint32_t hi2,
                     int32_t id1, int32_t id2, int32_t consume)
{
    int32_t ids[2] = { id1, id2 };
    input_mgr *m = self->input;
    int32_t mask[INPUT_RING_LEN];
    memset(mask, 0, sizeof mask);
    for (int k = 0; k < 2; k++) {
        if (input_ring_find(m, now, lo, hi, 1, ids[k], 0, 0, mask, 0) < 0) return 0;
        lo = 0; hi = hi2;
    }
    if (consume != 0)
        for (int k = 0; k < 2; k++) {
            int32_t want = (ids[k] == 5 || ids[k] == 6) ? 0 : ids[k];
            for (int i = 0; i < INPUT_RING_LEN; i++)
                if (m->ring[i]->id == want) m->ring[i]->id = 0;
        }
    return 1;
}

/* The walk-start delay: 10 ms (difficulty 0/1), 80 ms (2), else the forward id. */
static int32_t walk_delay(const wl_char *self, int32_t fwd)
{
    int32_t d = self->difficulty;
    if (d >= 0) {
        if (d <= 1) return 10;
        if (d == 2) return 80;
    }
    return fwd;
}

/* Cycle the targeted character `cur` through the world's 32 character slots by
 * `step` (1 or 31): the first alive one the spell accepts (0x47adb0). */
static void target_cycle(wl_char *self, wl_body *b, int32_t spell, int32_t cur, int32_t fwd, int step)
{
    int k;
    for (k = 0; k < 32 && cur != (int32_t)(uintptr_t)g_world->chars[k]; k++) ;
    int idx = k < 32 ? k : fwd;
    int hits = 0;
    wl_char *c;
    for (int tries = 0; ; ) {
        idx = (idx + step) % 32;
        c = g_world->chars[idx];
        if (c->base.alive != 0 && beh_47adb0(c, (int32_t)(intptr_t)self, (int32_t *)(spell), NULL, NULL, 0, 1, NULL) != 0 && ++hits >= 1)
            break;
        if (++tries >= 32) { b->target = 0; return; }
    }
    b->target = (int32_t)(uintptr_t)c;
    sfx_play(g_eng.sys, 1, 0, 0);
}

int char_ai(wl_char *self, wl_body *b, int32_t a3)
{
    uint32_t now = g_eng.tick();
    int32_t facing = b->facing;
    wl_cmd prev = self->cmd;
    cmd_clear(&self->cmd);
    int used_skill = 0, latched = 0, any_key = 0;

    wl_char *t = self->target;
    if (t != NULL && (t->base.alive != 1 || t->base.body->active != 1 || t->base.body->f7e != 0 ||
                      t->party->busy != 0))
        self->target = NULL;

    int32_t fwd = facing == 3 ? 2 : 4, back = facing == 3 ? 4 : 2;

    /* ── target cycling (DOWN / UP taps while a targeting spell is up) ── */
    wl_body *mb = self->base.body;
    if (self->f200 == 0 && self->fc788 != 0 && mb->spell != 0) {
        int32_t spell = mb->spell, cur = mb->target;
        if (cur != 0) {
            if (input_ring_find(self->input, now, 0, 200, 1, 3, 0, 0, NULL, 1) >= 0)
                target_cycle(self, b, spell, cur, fwd, 1);
            if (input_ring_find(self->input, now, 0, 200, 1, 1, 0, 0, NULL, 1) >= 0)
                target_cycle(self, b, spell, cur, fwd, 31);
        }
    }

    /* ── LEFT: walk 1 / run 5 ── */
    {
        int32_t win = KEYCFG()->dash_window;
        int dbl = char_ring_double(self, now, 0, win, win, 2, 2, 0) != 0;
        input_mgr *in = self->input;
        int held = in->axis_held[2] != 0, alt = 0;
        uint32_t t0 = (uint32_t)in->axis_held_b[2];
        if (in->axis_held[7] != 0) { t0 = (uint32_t)in->axis_held_b[7]; held = 1; alt = 1; }
        uint32_t delay = (uint32_t)walk_delay(self, fwd);
        int go = facing == 1 ? delay < now - t0 : now - t0 > 10u;
        if (!held) goto left_done;
        if (alt && KEYCFG()->run_mode == 2) {
            if (in->axis_held[9] != 0) goto left_run;
            if (prev.lr == 5) goto left_walk;
            goto left_check_walk;
        }
        if (prev.lr == 5 || dbl) goto left_run;
left_check_walk:
        if (prev.lr == 1 || go) goto left_walk;
        goto left_done;
left_walk:
        self->cmd.lr = 1;
        goto left_done;
left_run:
        self->cmd.lr = 5;
left_done:;
    }

    /* ── RIGHT: walk 2 / run 6 ── */
    {
        int32_t win = KEYCFG()->dash_window;
        int dbl = char_ring_double(self, now, 0, win, win, 4, 4, 0) != 0;
        input_mgr *in = self->input;
        int held = in->axis_held[3] != 0, alt = 0;
        uint32_t t0 = (uint32_t)in->axis_held_b[3];
        if (in->axis_held[8] != 0) { t0 = (uint32_t)in->axis_held_b[8]; held = 1; alt = 1; }
        uint32_t delay = (uint32_t)walk_delay(self, fwd);
        int go = facing == 3 ? delay < now - t0 : now - t0 > 10u;
        if (!held) goto right_done;
        if (alt && KEYCFG()->run_mode == 2) {
            if (in->axis_held[10] != 0) goto right_run;
            if (prev.lr == 6) goto right_walk;
            goto right_check_walk;
        }
        if (prev.lr == 6 || dbl) goto right_run;
right_check_walk:
        if (prev.lr == 2 || go) goto right_walk;
        goto right_done;
right_walk:
        self->cmd.lr = 2;
        goto right_done;
right_run:
        self->cmd.lr = 6;
right_done:;
    }

    /* ── DOWN 10 / UP 11: a fresh press, a held-over command, or held > 240 ms ── */
    {
        int down_hit = input_ring_find(self->input, now, 10, 800, 1, 3, 0, 0, NULL, 0) >= 0;
        input_mgr *in = self->input;
        if (in->axis_held[1] != 0 && (prev.ud == 10 || down_hit || now - (uint32_t)in->axis_held_b[1] > 0xf0u))
            self->cmd.ud = 10;
        int up_miss = input_ring_find(in, now, 10, 800, 1, 1, 0, 0, NULL, 0) < 0;
        in = self->input;
        if (in->axis_held[0] != 0 && (prev.ud == 11 || !up_miss || now - (uint32_t)in->axis_held_b[0] > 0xf0u))
            self->cmd.ud = 11;
    }

    /* ── the up-assist: probe up to 4 integrator steps with an UP command (0x442a70
     * on two alternating scratch bodies, 0x440f40 judging each) ── */
    if (KEYCFG()->f078 == 1 && self->cmd.ud == 0 && self->up_assist != 0) {
        wl_body probe[2];
        uint8_t frames[2][0x50];
        probe[0].frames = frames[0];
        probe[1].frames = frames[1];
        wl_body *in_b = self->base.body, *out_b = NULL;
        int32_t flag = 0;
        int32_t n = 1;
        for (;;) {
            out_b = out_b == &probe[0] ? &probe[1] : &probe[0];
            wl_cmd c;
            cmd_clear(&c);
            c.ud = 11;
            char_integrate(self, &c, in_b, out_b, 2, n);
            if (char_hazard_probe(self, (uint16_t)n, out_b, &flag) != 0) break;
            ++n;
            in_b = out_b;
            if ((uint16_t)n >= 5) {
                if (flag != 0) self->cmd.ud = 11;
                break;
            }
        }
    }

    /* ── jump 7 / drop through 9 (DOWN held) ── */
    if (input_ring_find(self->input, now, 0, 800, 1, 7, 0, 0, NULL, 1) >= 0)
        self->cmd.jump = self->input->axis_held[1] != 0 ? 9 : 7;
    {
        input_mgr *in = self->input;
        if (in->axis_held[5] != 0 && now - (uint32_t)in->axis_held_b[5] > 200u) {   /* attack held */
            g_eng.skill_479f90(self, b, a3);
            self->cmd.act = 14;
        }
    }
    if (b->f66 != 0 && b->f68 == 0 && input_ring_hit(self->input, now, 0, 400, 1, 9, 1))
        self->cmd.act = 15;

    /* ── combo skills ── */
    if (self->cmd.act == 0 && self->n_combo != 0) {
        wl_combo *cb;
        for (int i = 0; ; ) {
            cb = &self->combo[i];
            if ((cb->cond == 0 || char_skill_avail(self, (const wl_skill *)(intptr_t)cb->cond, b, a3, 0) != 0) &&
                input_combo_match(self->input, cb, now, fwd, back, 1, 0) != 0)
                break;
            if (++i >= self->n_combo) goto combos_done;
        }
        if (cb->direct != 0) cmd_set_skill(&self->cmd, (int16_t)cb->skill, (int16_t)cb->arg, 0);
        else cmd_set_skill_kind(&self->cmd, (int16_t)cb->skill, 210, (int16_t)cb->arg, 0);
    }
combos_done:

    /* ── the attack button tap (latched for the member skill below) ── */
    if (KEYCFG()->skill_keys != 0) {
        if (input_ring_find(self->input, now, 0, 200, 0, 9, 0, 0, NULL, 1) >= 0 &&
            input_ring_find(self->input, now, 0, 500, 1, 9, 0, 0, NULL, 1) >= 0)
            latched = 1;
    } else if (input_ring_find(self->input, now, 0, 200, 1, 9, 0, 0, NULL, 1) >= 0) {
        latched = 1;
    }

    /* ── the skill-slot keys ── */
    int32_t mode = self->skill_input;
    if (self->base.code == 50010 && self->party->f466 == 0) mode = 2;
    if (mode == 1) {
        for (int k = 0; k < 5; k++)
            if (input_ring_hit(self->input, now, 0, 200, 1, 24 + k, 1) &&
                party_slot_get(self->party) != k &&
                member_skill_at((const uint8_t *)self->party, self->party->slot_mode, (uint16_t)k) != NULL) {
                party_slot_set(self->party, (int16_t)k);
                used_skill = 1;
            }
        for (int k = 0; k < 5; k++)
            if (g_eng.skill_47a7f0(self, 29 + k, now, 5 + k) != 0) used_skill = 1;
        int sel = 0;
        int hit10 = input_ring_hit(self->input, now, 0, 200, 1, 10, 1);
        if (KEYCFG()->skill_keys != 1) goto no_sel;
        if (input_ring_hit(self->input, now, 0, 200, 1, 13, 1)) { hit10 = 1; any_key = 1; }
        if (input_ring_hit(self->input, now, 0, 200, 1, 12, 1)) {
            sel = 1;
            any_key = 1;
        } else {
no_sel:
            if (!hit10) goto sel_done;
        }
        if (g_eng.skill_47a300(self, sel, 1) != 0) {
            if (g_eng.skill_47a420(self, 0, 0) == 0) party_slot_set(self->party, 0);
            used_skill = 1;
        }
sel_done:
        if (KEYCFG()->skill_keys == 2) {
            if (g_eng.skill_47a7f0(self, 13, now, 5) != 0) { used_skill = 1; any_key = 1; }
            if (g_eng.skill_47a7f0(self, 12, now, 0) != 0) { used_skill = 1; any_key = 1; }
        }
        int sel2 = 0;
        int want = self->n_member_skill == 0 ? latched != 0 : 0;
        if (input_ring_hit(self->input, now, 0, 200, 1, 15, 1)) { want = 1; any_key = 1; }
        if (input_ring_hit(self->input, now, 0, 200, 1, 14, 1)) {
            sel2 = 1;
            any_key = 1;
        } else if (!want) {
            goto cast_if_used;
        }
        if (g_eng.skill_47a420(self, sel2, 1) != 0) goto cast;
cast_if_used:
        if (used_skill == 0) goto slots_done;
cast:
        if (any_key) input_ring_hit(self->input, now, 0, 500, 1, 9, 1);
        if (b->f66 != 0 || self->cmd.act == 14) self->cmd.act = 15;
        sfx_play(g_eng.sys, 0, 0, 0);
        if (self->base.code >= 0xc35b && self->base.code <= 0xc35c) {
            if (b->pose == 10) b->f58 = 2;
            else cmd_set_skill_kind(&self->cmd, 10, 100, 0, 0);
        }
    }
slots_done:

    /* ── the party member's own skill on the latched tap ── */
    if (self->n_member_skill != 0 && latched != 0) {
        int i;
        for (i = 0; self->member_skill[i].key != self->party->member; )
            if (++i >= self->n_member_skill) goto member_done;
        cmd_set_member_skill(&self->cmd, (int16_t)self->member_skill[i].val, 210);
    }
member_done:;

    /* ── jump held: 8, or 9 when the previous tick dropped through ── */
    int result = (int)(intptr_t)self->input;
    if (self->input->axis_held[4] != 0) {
        result = self->cmd.jump;
        if (result == 0) self->cmd.jump = (prev.jump == 9) + 8;
    }
    return result;
}

/* ── FUN_00440f40 / FUN_004411e0 ──────────────────────────────────────────── */

int32_t body_frame_box(uint16_t r, const wl_body *b, int32_t box[4])
{
    const int32_t *f = (const int32_t *)b->frames + 5u * r;
    int32_t dx = f[0];
    if (b->facing == 3) dx = -dx;
    box[0] = dx + b->x + b->w / 2 - f[2] / 2;
    box[2] = f[2];
    box[3] = f[3];
    box[1] = f[1] + (b->y + b->h / 2 - f[3] / 2);
    return f[1];
}

int char_hazard_probe(wl_char *self, uint16_t n, const wl_body *b, int32_t *out)
{
    if (self->party->busy != 0 || self->f14234 != 0 || b->f7d != 0) return 0;
    int looking_up = b->state == 5 && self->up_assist != 0;
    const wl_list *L = g_world->list_2788;
    if (L->count == 0) return 0;
    for (uint16_t i = 0; i < L->count; L = g_world->list_2788, i++) {
        const wl_entity *e = L->items[i];
        if (e == &self->base) continue;
        int32_t fs = self->base.faction, fe = e->faction;
        if (!((fs != 0 && fe != 0 && (fs == 3 || fe == 3 || fs != fe)) || e->coll_off == 1)) continue;
        uint16_t nr = self->base.nrects;
        const uint8_t *rects = self->base.rects;
        if (e->nbody <= n) continue;
        const wl_body *eb = &e->body[n];
        if (!eb->active || eb->nhit == 0) continue;
        const int32_t *hb = NULL;
        for (uint16_t k = 0; k < e->body[n].nhit; k++) {
            const int32_t *h = e->body[n].hit[k];
            if (rects != NULL) {
                for (uint16_t r = 0; r < nr; r++) {
                    int32_t box[4];
                    body_frame_box(r, b, box);
                    if (h[5] <= box[0] + box[2] && h[5] + h[7] > box[0] &&
                        h[6] <= box[1] + box[3] && h[6] + h[8] > box[1]) { hb = h; break; }
                }
                if (hb) break;
            } else if (h[5] <= b->x + b->w && h[5] + h[7] > b->x &&
                       h[6] <= b->y + b->h && h[6] + h[8] > b->y + b->margin) {
                hb = h;
                break;
            }
        }
        if (hb == NULL) continue;
        if (hb[15] != 0) {
            int j;
            for (j = 0; j < 8; j++)
                if (self->recent_hit[j].id == hb[15] && self->recent_hit[j].t > n + 30) break;
            if (j < 8) continue;            /* already taken: next entity */
        }
        int32_t kind = hb[4];
        if (hb[3] != 0) {
            switch (self->hazard_mode) {
            case 1: if (kind != 0) return 1; break;
            case 2: if (kind != 1) return 1; break;
            case 3: break;
            default: return 1;
            }
        }
        if (looking_up) {
            if (out != NULL) *out = 1;
            continue;
        }
        return 1;
    }
    return 0;
}
