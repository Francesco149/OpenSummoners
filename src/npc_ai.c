/* npc_ai.c — FUN_0043f880 and its helper cluster.  See npc_ai.h.
 *
 * Transcribed from the decompile; every 0x440f40 call is thiscall(self, n, body, &out)
 * (call-site pushes — IDA renders a phantom edx argument).  The command block is
 * zeroed in full before use: retail leaves its last word (+0x1e) as stack garbage and
 * rhost's uninit-fix (quirk #123, site C) zeroes it — the port reproduces the fixed run.
 * Scratch body arrays come from the engine heap (g_eng.op_new) like retail's. */
#include "npc_ai.h"

#include <stddef.h>
#include <string.h>

#include "char_ai.h"
#include "engine_calls.h"
#include "entity_move.h"
#include "map_grid.h"
#include "rng.h"

#define NPC_SCRATCH 64               /* bodies per scratch block (0xa500 bytes) */
#define NPC_STEPS   32               /* the long look-ahead simulates bodies 1..31 */

static int32_t gi32(size_t off) { int32_t v; memcpy(&v, g_world->grid + off, 4); return v; }
static uint16_t gu16(size_t off) { uint16_t v; memcpy(&v, g_world->grid + off, 2); return v; }
static int32_t clampi(int32_t v, int32_t hi) { if (v < 0) v = 0; return hi >= v ? v : hi; }
static int32_t region_d(int32_t col, int32_t row) { return gu16(MG_REGION_D + 2u * (uint32_t)(row + (col << 7))); }

/* ── box helpers ──────────────────────────────────────────────────────────── */

static const wl_body *part_of(const wl_entity *e, uint16_t part) { return &e->body[part < e->nbody ? part : 0]; }
int32_t part_bottom(const wl_entity *e, uint16_t part) { const wl_body *p = part_of(e, part); return p->h + p->y - 1; }
int32_t part_left(const wl_entity *e, uint16_t part) { return part_of(e, part)->x; }
int32_t part_right(const wl_entity *e, uint16_t part) { const wl_body *p = part_of(e, part); return p->x + p->w; }
int32_t part_center_x(const wl_entity *e, uint16_t part) { const wl_body *p = part_of(e, part); return p->x + p->w / 2; }
int32_t part_center_y(const wl_entity *e, uint16_t part)
{ const wl_body *p = part_of(e, part); return p->margin + p->y + (p->h - p->margin) / 2; }
int32_t part_width(const wl_entity *e, uint16_t part) { return part_of(e, part)->w; }
static int32_t part_top(const wl_entity *e, uint16_t part) { const wl_body *p = part_of(e, part); return p->y + p->margin; }

int grid_c_query(const uint8_t *grid, int32_t x, int32_t y, int32_t *out)
{
    int32_t v; memcpy(&v, grid + MG_DIM0, 4);
    int32_t col = clampi(x / 3200, v - 1);
    memcpy(&v, grid + MG_DIM1, 4);
    int32_t row = clampi(y / 3200, v - 1);
    size_t off = MG_REGION_C + 12u * (uint32_t)(row + (col << 7));
    int32_t r, c4;
    memcpy(&r, grid + off, 4); memcpy(&c4, grid + off + 4, 4);
    if (r == 1) {
        if (y % 3200 <= 1600) { if (out) *out = 0; return 0; }
        if (out) *out = c4;
        return 1;
    }
    if (out) *out = c4;
    return r;
}

const int32_t *body_hitbox_hit(const wl_char *e, const wl_body *b, int32_t part, int32_t use_frames, uint16_t nframes)
{
    if (e->base.nbody <= part) return NULL;
    const wl_body *eb = &e->base.body[part];
    if (!eb->active) return NULL;
    uint16_t nh = eb->nhit;
    for (uint16_t k = 0; k < nh; k++) {
        const int32_t *h = e->base.body[part].hit[k];
        if (use_frames != 0) {
            for (uint16_t r = 0; r < nframes; r++) {
                int32_t box[4];
                body_frame_box(r, b, box);
                if (h[5] <= box[0] + box[2] && h[5] + h[7] > box[0] && h[6] <= box[1] + box[3] && h[6] + h[8] > box[1])
                    return h;
            }
        } else if (h[5] <= b->x + b->w && h[5] + h[7] > b->x && h[6] <= b->y + b->h && h[6] + h[8] > b->y + b->margin) {
            return h;
        }
    }
    return NULL;
}

int char_hit_fresh(const wl_char *self, const int32_t *hit, uint16_t n)
{
    if (hit[15] == 0) return 1;
    for (int j = 0; j < 8; j++)
        if (self->recent_hit[j].id == hit[15] && self->recent_hit[j].t > n + 30) return 0;
    return 1;
}

int char_hit_kind_ok(const wl_char *self, int32_t gate, int32_t kind)
{
    if (gate == 0) return 1;
    if (self->hazard_mode == 1) return kind == 0;
    if (self->hazard_mode != 2) return self->hazard_mode == 3;
    return kind == 1;
}

int entity_blocks(const wl_entity *e, const wl_entity *other, uint16_t part, int32_t dir, int16_t pass, int32_t nocollide)
{
    if (e->nbody <= part || !e->body[part].active || e->body[part].nocollide) return 0;
    switch (e->coll_mode) {
    case 0: return 0;
    case 1: if (nocollide != 0) return 0; break;
    case 2:
        if (nocollide != 0 || e->coll_off != 0 || other->coll_off != 0) return 0;
        if (e->faction == 0 || other->faction == 0) return 0;
        if (e->faction != 3 && other->faction != 3 && e->faction == other->faction) return 0;
        break;
    default: break;
    }
    switch (dir) {
    case 0:         return e->coll_kind == 3;
    case 1: case 3: return e->coll_kind == 1 || e->coll_kind == 3;
    case 2:         return e->coll_kind == 2 ? pass == 0 : e->coll_kind == 3;
    default:        return 1;
    }
}

int npc_blocked(const wl_char *self, const wl_body *b, uint16_t part)
{
    int32_t top = b->y + b->margin, bot = b->h + b->y - 1, x = b->x;
    const wl_list *L = g_world->collide;
    uint16_t n = L->count;
    for (uint16_t i = 0; i < n; i++) {
        const wl_entity *e = L->items[i];
        if (e == &self->base) continue;
        const wl_body *eb = part_of(e, part);
        if (top >= eb->h + eb->y - 1 || bot <= eb->y + eb->margin) continue;
        if (x >= eb->x + eb->w || x + b->w <= eb->x) continue;
        int nc = b->nocollide;
        if (e->nbody <= part || !e->body[part].active || e->body[part].nocollide) continue;
        int32_t m = e->coll_mode;
        if (m == 0) continue;
        if (m == 1) { if (nc != 0) continue; }
        else if (m == 2) {
            int32_t fe = e->faction, fs = self->base.faction;
            if (!(nc == 0 && e->coll_off == 0 && self->base.coll_off == 0 && fe != 0 && fs != 0 &&
                  (fe == 3 || fs == 3 || fe != fs)))
                continue;
        }
        if (e->coll_kind == 1 || e->coll_kind == 3) return 1;
    }
    return 0;
}

int npc_floor_ahead(wl_char *self, const wl_body *b, int32_t x, int32_t strict, int32_t check_floor, int32_t depth)
{
    int32_t dim0 = gi32(MG_DIM0), dim1 = gi32(MG_DIM1);
    int32_t col = x / 3200;
    int32_t top_row = clampi(b->y / 3200, dim1 - 1);
    int32_t h = b->h;
    int32_t row = clampi((h + b->y - 1) / 3200, dim1 - 1);
    if (strict != 0) {
        if (x < 0 || col >= dim0) return 0;
    } else {
        col = clampi(col, dim0 - 1);
    }
    if (region_d(col, top_row) == 1) return 0;
    if (region_d(col, row) == 1) --row;
    if (check_floor == 0) return 1;
    const wl_entity *ride = b->attach;
    if (ride == NULL) {
        int32_t r = row + 1;
        if (row + 1 > row + depth) return 0;
        for (;;) {
            if (r >= dim1) return 0;
            if (region_d(col, r) != 0) break;
            if (++r > row + depth) return 0;
        }
        if (self->nav_c_check != 0 && self->fc7ec == 0 && self->party->f45e == 0) {
            int32_t fy = 3200 * r - h;
            int32_t ccol = clampi(3200 * col / 3200, dim0 - 1);
            int32_t crow = clampi(fy / 3200, dim1 - 1);
            size_t off = MG_REGION_C + 12u * (uint32_t)(crow + (ccol << 7));
            int32_t c0 = gi32(off), c4 = gi32(off + 4);
            if (c0 != 1) {
                if (c0 == 0) return 1;
                return c4 == 0 || c4 == 2;
            }
            if (fy % 3200 > 1600) return c4 == 0 || c4 == 2;
        }
        return 1;
    }
    switch (ride->ride_mode) {
    case 0: case 1: if (self->platform != ride) return 1; break;
    case 3:         return 1;
    default:        break;
    }
    return x <= ride->body[0].x + ride->body[0].w && x >= ride->body[0].x;
}

/* ── scratch bodies ───────────────────────────────────────────────────────── */

static wl_body *probe_alloc(wl_char *self)
{
    wl_body *blk = g_eng.op_new(0xa500);
    if (self->base.rects != NULL)
        for (int i = 0; i < NPC_SCRATCH; i++) blk[i].frames = g_eng.op_new(0x50);
    return blk;
}

void npc_probe_free(wl_char *self, wl_body *block, uint16_t n)
{
    if (self->base.rects != NULL && n != 0)
        for (uint16_t i = 0; i < n; i++)
            if (block[i].frames != NULL) { g_eng.op_delete(block[i].frames); block[i].frames = NULL; }
    if (block != NULL) g_eng.op_delete(block);
}

/* simulate `cmd` on bodies 1..steps-1 of a fresh block, chained from the live body */
static void simulate(wl_char *self, wl_cmd *cmd, wl_body *blk, int32_t steps, int16_t switch_at, int32_t switch_lr)
{
    wl_body *prev = self->base.body;
    for (int32_t i = 1; (uint16_t)i < steps; i++) {
        if (switch_at != 0 && switch_at == (int16_t)(uint16_t)i) cmd->lr = switch_lr;
        g_eng.integrate_442a70(self, cmd, prev, &blk[(uint16_t)i], 2, i);
        prev = &blk[(uint16_t)i];
    }
}

/* ── FUN_004412d0 ─────────────────────────────────────────────────────────── */

int npc_try_cmd(wl_char *self, wl_cmd *cmd, int32_t steps, int32_t check_hits, int16_t switch_at,
                int32_t switch_lr, int32_t need_hit)
{
    int ok = 0;
    if (self->no_sim != 0) return 1;
    wl_body *blk = probe_alloc(self);
    simulate(self, cmd, blk, steps, switch_at, switch_lr);
    if (self->base.rects == NULL) {
        const wl_body *last = &blk[steps - 1];
        int32_t top = last->y + last->margin, bot = last->h + last->y - 1, l = last->x, r = last->x + last->w;
        const wl_list *L = g_world->collide;
        for (uint16_t i = 0; i < L->count; L = g_world->collide, i++) {
            const wl_entity *e = L->items[i];
            if (e == &self->base) continue;
            const wl_body *eb = part_of(e, (uint16_t)(steps - 1));
            if (top < eb->h + eb->y - 1 && bot > eb->y + eb->margin && l < eb->x + eb->w &&
                r > part_left(e, (uint16_t)(steps - 1)) &&
                entity_blocks(e, &self->base, (uint16_t)(steps - 1), 3, 0, last->nocollide))
                goto done;
        }
    }
    {
        const wl_body *me = self->base.body, *last = &blk[steps - 1];
        int32_t dx = me->x - last->x;
        if (dx < 0) dx = -dx;
        if (check_hits == 0 || dx >= 400 || me->facing != last->facing) {
            int hit = 0;
            for (int32_t s = 1; s < steps; s++) {
                const wl_body *sb = &blk[s];
                if (self->party->busy != 0 || self->f14234 != 0 || sb->f7d != 0) continue;
                int looking_up = sb->state == 5 && self->up_assist != 0;
                const wl_list *L = g_world->list_2788;
                for (uint16_t i = 0; i < L->count; L = g_world->list_2788, i++) {
                    const wl_char *e = (const wl_char *)L->items[i];
                    if (&e->base == &self->base) continue;
                    if (faction_relation(self->base.faction, e->base.faction) != 1 && e->base.coll_off != 1) continue;
                    const int32_t *hb = body_hitbox_hit(e, sb, s, self->base.rects != NULL, self->base.nrects);
                    if (hb != NULL && char_hit_fresh(self, hb, (uint16_t)s) != 0) {
                        if (!char_hit_kind_ok(self, hb[3], hb[4]) || !looking_up) goto done;
                        hit = 1;
                    }
                }
            }
            if (need_hit == 0 || hit) ok = 1;
        }
    }
done:
    npc_probe_free(self, blk, NPC_SCRATCH);
    return ok;
}

/* the look-up / UP look-ahead (0x43f880 action 3 and its tail): 1 = an UP command is
 * worth taking (a hitbox it reaches while looking up), 0 = not; *took = the block was run */
static int up_lookahead(wl_char *self, wl_cmd *cmd)
{
    int good = 0;
    wl_body *blk = probe_alloc(self);
    simulate(self, cmd, blk, NPC_STEPS, 0, 0);
    if (self->base.rects != NULL || npc_blocked(self, &blk[NPC_STEPS - 1], NPC_STEPS - 1) == 0) {
        int32_t flag = 0;
        for (uint16_t n = 1; ; ) {
            if (char_hazard_probe(self, n, &blk[n], &flag) != 0) break;
            if (++n >= NPC_STEPS) { if (flag != 0) good = 1; break; }
        }
    }
    if (self->base.rects != NULL)
        for (int i = 0; i < NPC_SCRATCH; i++)
            if (blk[i].frames != NULL) { g_eng.op_delete(blk[i].frames); blk[i].frames = NULL; }
    g_eng.op_delete(blk);
    return good;
}

/* ── FUN_0043f880 ─────────────────────────────────────────────────────────── */

int npc_navigate(wl_char *self, int32_t tx, int32_t arrive, wl_body *b, int32_t grounded, int32_t calm,
                 int32_t mode, int32_t ty, int32_t no_run, int32_t want_jump)
{
    int32_t dim0 = gi32(MG_DIM0), dim1 = gi32(MG_DIM1);
    int can_act, head_clear, crouch_ok = 0, jump_ok = 0, drop = 0, near = 0, sim = 0;
    int up_done = 0;
    int left_ok, right_ok;
    switch (b->state) {
    case 0: case 1: case 4: can_act = 1; break;
    case 2:                 can_act = b->sub == 2; break;
    default:                can_act = 0; break;
    }

    /* headroom: no region-D floor in the 2 rows above the head (either side column) */
    if (self->can_jump != 0) {
        int32_t c0 = clampi(b->x / 3200, dim0 - 1);
        int32_t c1 = clampi((b->w + b->x - 1) / 3200, dim0 - 1);
        int32_t row = b->y / 3200;
        if (self->move_mode == 3) {
            head_clear = 1;
        } else {
            int32_t r = row - 1;
            head_clear = 0;
            for (int k = 1; ; ) {
                if (region_d(c0, clampi(r, dim1 - 1)) == 1) break;
                if (region_d(c1, clampi(r, dim1 - 1)) == 1) break;
                --r;
                if (++k > 2) { head_clear = 1; break; }
            }
        }
    } else {
        head_clear = 0;
    }

    int32_t x = b->x;
    int32_t dist = x - tx < 0 ? tx - x : x - tx;
    int look_up = (can_act || b->state == 5) && self->up_assist != 0;
    {
        int32_t mm = self->move_mode;
        int strict = mm <= 0 || (mm > 2 && mm != 3);
        left_ok = mm == 3 ? 1 : npc_floor_ahead(self, b, x - 1600, 0, strict, 5);
        right_ok = mm == 3 ? 1 : npc_floor_ahead(self, b, b->w + b->x + 1599, 0, strict, 5);
    }
    if (self->walk_max / 2 < b->vel_x) {
        int32_t ahead = self->walk_max >= b->vel_x ? 3200 : 6400;
        int32_t mm = self->move_mode;
        int check = mm <= 0 || (mm > 2 && mm != 3);
        if (mm == 3) near = 0;
        else if (b->facing == 1) near = !npc_floor_ahead(self, b, b->x + b->w + ahead - 1, 0, check, 5);
        else near = !npc_floor_ahead(self, b, b->x - ahead, 0, check, 5);
    }

    int32_t mm = self->move_mode;
    if (mm == 0) {
        sim = 1;
        jump_ok = !(self->air_jumps >= self->max_air_jumps && grounded == 0) && can_act;
        crouch_ok = grounded != 0 && (can_act || b->state == 2) && self->can_crouch != 0;
        if (head_clear && b->attach != NULL && b->attach == self->platform && self->nav_c870 == 10) head_clear = 0;
    } else if (mm > 0 && mm <= 3) {
        int32_t col0 = b->x / 3200;
        int32_t c0 = clampi(col0, dim0 - 1);
        int32_t col1 = (b->w + b->x - 1) / 3200;
        int32_t c1 = clampi(col1, dim0 - 1);
        int32_t bottom = b->h + b->y;
        int32_t frow = (b->h + b->y - 1) / 3200, r = frow;
        do ++r; while (r < dim1 && region_d(c0, r) == 0 && region_d(c1, r) == 0);
        int32_t gap = 3200 * r - b->h - b->y;
        if (mode == 1) {
            int32_t aim = ty - self->nav_drop;
            jump_ok = can_act; head_clear = 1; sim = 1; can_act = 0;
            if (bottom < ty - self->nav_drop - 3200 || (bottom < aim - 800 && b->vvel < 5000)) drop = 1;
            if (bottom > aim + 3200 || (bottom > aim + 800 && b->vvel > -5000)) want_jump = 1;
        } else if (mode != 2) {
            sim = 0;
            jump_ok = can_act;
            if (gap < self->nav_drop) want_jump = 1;
            if (grounded != 0) {
                can_act = (can_act || b->state == 2) && self->can_crouch != 0;
            } else if (gap > 3200) {
                can_act = 1;
                if (gap > self->nav_fall) drop = 1;
            } else {
                can_act = 0;
            }
            if (self->target != NULL) {
                const wl_body *mb = self->base.body;
                const wl_body *tb = self->target->base.body;
                int32_t rel = tb->y + tb->margin - mb->y - mb->h + 1;
                if (self->nav_rise > rel) { want_jump = 1; drop = 0; }
                if (self->nav_rise + 3200 < rel) { want_jump = 0; drop = 1; }
            }
            if (self->nav_low_bridge != 0 && gap < 3200) { want_jump = 1; drop = 0; }
        } else {
            jump_ok = can_act; head_clear = 1; sim = 1; can_act = 0;
            if (gap < ty + 3200 || (gap < ty + 800 && b->vvel > -10000)) want_jump = 1;
            if (gap > ty - 3200 || (gap > ty - 800 && b->vvel < 10000)) drop = 1;
        }
        if (self->float_h > 0 && self->float_h <= gap) head_clear = 0;
        if (self->nav_c_check != 0 && self->fc7ec == 0 && self->party->f45e == 0 && self->nav_drop > 0) {
            int32_t cx = 3200 * ((col0 + col1) / 2), c4 = 0;
            int32_t d = 0;
            for (;;) {
                if (!(grid_c_query(g_world->grid, cx, 3200 * (frow + d / 3200), &c4) == 0 || c4 == 0 || c4 == 2)) {
                    can_act = 0; drop = 0; want_jump = 1;
                    break;
                }
                d += 3200;
                if (d >= self->nav_drop) break;
            }
        }
        crouch_ok = can_act;
    }
    if (grounded == 0 && self->f15a24 == 0) look_up = 0;
    if (mm != 0 && !(mm > 0 && mm <= 3)) crouch_ok = can_act;

    /* the direction candidates */
    int32_t dir[5];
    if (arrive > dist) {
        dir[0] = 0; dir[1] = 1; dir[2] = 2; dir[3] = 5; dir[4] = 6;
    } else {
        int toward_left = b->x > tx;
        int choose;
        if (sim && ((self->walk_max < b->vel_x && dist <= 8000) || (self->walk_max / 2 < b->vel_x && dist <= 3200) || near))
            choose = b->facing == 3 ? 1 : 2;           /* keep going (brake) */
        else
            choose = toward_left ? (dist <= 9600 ? 3 : 4) : (dist <= 9600 ? 5 : 6);
        switch (choose) {
        case 1: case 6: dir[0] = 6; dir[1] = 2; dir[2] = 0; dir[3] = 1; dir[4] = 5; break;
        case 2: case 4: dir[0] = 5; dir[1] = 1; dir[2] = 0; dir[3] = 2; dir[4] = 6; break;
        case 3:         dir[0] = 1; dir[1] = 5; dir[2] = 0; dir[3] = 2; dir[4] = 6; break;
        default:        dir[0] = 2; dir[1] = 6; dir[2] = 0; dir[3] = 1; dir[4] = 5; break;
        }
    }

    /* the action list */
    int32_t act[32];
    int na = 0;
    if (self->nav_chase == 1 && self->target != NULL && self->nav_skill != 0) {
        if ((b->x > tx + 8000 && b->facing == 1) || (b->x < tx - 8000 && b->facing == 3)) act[na++] = 5;
    }
    if (head_clear && want_jump) act[na++] = 4;
    if (drop) act[na++] = 2;
    act[na++] = 1;
    if (look_up && b->state == 5) act[na++] = 3;
    if (crouch_ok) act[na++] = 2;
    if (look_up && calm == 0) {
        int32_t chance = self->nav_jump_chance;
        wl_party *p = self->party;
        if (1000 * p->hp / (p->hp_max + p->hp_bonus1 + p->hp_bonus2) < 300) chance /= 2;
        if (chance > 1000 * (int32_t)rng_rand() / 0x8000) act[na++] = 3;
    }
    if (self->nav_skill != 0) act[na++] = 5;
    if (self->nav_skill2 != 0) {
        int32_t lo = self->nav_range_lo, hi = self->nav_range_hi;
        int32_t d2 = b->facing != 1 ? b->x - tx : tx - b->x;
        if (lo == hi || (d2 >= lo && d2 <= hi)) act[na++] = 6;
    }
    if (head_clear) act[na++] = 4;

    wl_cmd cmd;
    int result = 1;
    for (uint16_t di = 0; di < 5; di++) {
        int run = 0, toward = 0, fwd = 0, back = 0;
        int32_t d = dir[di];
        switch (d) {
        case 5: run = 1; /* fall through */
        case 1:
            if (!left_ok) goto next_dir;
            toward = 1; fwd = b->facing == 3; back = !fwd;
            break;
        case 6: run = 1; /* fall through */
        case 2:
            if (!right_ok) goto next_dir;
            toward = 1; fwd = b->facing == 1; back = !fwd;
            break;
        default:
            toward = 0; fwd = 0; back = 0;
            goto try_actions;
        }
        if (run && !(self->can_run != 0 && no_run == 0)) goto next_dir;
try_actions:
        for (int ai = 0; ai < na; ai++) {
            memset(&cmd, 0, sizeof cmd);
            switch (act[ai]) {
            case 1:
                cmd.lr = d;
                if ((self->nav_sim_off == 0 || self->f14288 != 0) && npc_try_cmd(self, &cmd, 32, toward, 0, 0, 0) == 0) break;
                goto accept;
            case 2:
                if (grounded != 0 && d != 0 && drop == 0) break;
                cmd.lr = d; cmd.ud = 10;
                if ((self->nav_sim_off == 0 || self->f14288 != 0) && npc_try_cmd(self, &cmd, 32, 0, 0, 0, 0) == 0) break;
                goto accept;
            case 3:
                if (up_done) break;
                up_done = 1;
                cmd.ud = 11;
                if (self->no_sim != 0) goto accept;
                if (up_lookahead(self, &cmd) == 0) break;
                goto accept;
            case 4: {
                if (self->air_max != 0 && run) break;
                int32_t j;
                if (b->state <= 1) { if (!jump_ok) break; j = 7; }
                else { if (b->state != 3) break; j = 8; }
                cmd.jump = j; cmd.lr = d;
                if ((self->nav_sim_off != 0 && self->f14288 == 0) || npc_try_cmd(self, &cmd, 32, 0, 0, 0, 0) != 0) goto accept;
                if (!toward) break;
                memset(&cmd, 0, sizeof cmd);
                cmd.jump = j;
                if (self->no_sim != 0) goto accept;
                {
                    wl_body *blk = probe_alloc(self);
                    simulate(self, &cmd, blk, NPC_STEPS, 0, 0);
                    int good = 0;
                    if (self->base.rects == NULL) {
                        const wl_body *l = &blk[NPC_STEPS - 1];
                        int32_t top = l->y + l->margin, bot = l->h + l->y - 1, lx = l->x, rx = l->x + l->w;
                        const wl_list *L = g_world->collide;
                        for (uint16_t i = 0; i < L->count; L = g_world->collide, i++) {
                            const wl_entity *e = L->items[i];
                            if (e == &self->base) continue;
                            if (top < part_bottom(e, NPC_STEPS - 1) && bot > part_top(e, NPC_STEPS - 1) &&
                                lx < part_right(e, NPC_STEPS - 1) && rx > part_left(e, NPC_STEPS - 1) &&
                                entity_blocks(e, &self->base, NPC_STEPS - 1, 3, 0, l->nocollide))
                                goto jump_done;
                        }
                    }
                    {
                        int32_t flag = 0;
                        for (uint16_t n = 1; ; ) {
                            if (char_hazard_probe(self, n, &blk[n], &flag) != 0) break;
                            if (++n >= NPC_STEPS) { good = flag != 0; break; }
                        }
                    }
jump_done:
                    npc_probe_free(self, blk, NPC_SCRATCH);
                    if (!good) break;
                }
                goto accept;
            }
            case 5:
            case 6: {
                int32_t sk = act[ai] == 5 ? self->nav_skill : self->nav_skill2;
                int32_t arg = act[ai] == 5 ? self->nav_skill_arg : self->nav_skill2_arg;
                if (act[ai] == 5 ? back == 0 : (fwd == 0 || run == 0)) break;
                for (int32_t k = 0; k < self->n_skills; k++)
                    if (self->skills[k].id == (int32_t)(uint16_t)sk) {
                        if (g_eng.skill_ok_402a60(self, (int32_t)(intptr_t)&self->skills[k], b, grounded, 1) == 0) goto rejected;
                        break;
                    }
                cmd.skill = (uint16_t)sk; cmd.skill_arg = (uint16_t)arg; cmd.skill_kind = 210;
                if ((self->nav_sim_off == 0 || self->f14288 != 0) && npc_try_cmd(self, &cmd, 40, 1, 0, 0, 0) == 0) break;
                goto accept;
            }
            default:
                break;
            }
rejected:
            result = 0;
        }
next_dir:
        result = 0;
    }
    if (look_up && !up_done) {
        memset(&cmd, 0, sizeof cmd);
        cmd.ud = 11;
        if (self->no_sim != 0) goto accept;
        if (up_lookahead(self, &cmd) != 0) goto accept;
    }
    memset(&cmd, 0, sizeof cmd);
accept:
    self->cmd = cmd;
    return result;
}
