/* scene_cast.c — the cutscene cast step + its path search.  See scene_cast.h.  Transcribed from the asm. */
#include "scene_cast.h"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "char_ai.h"          /* cmd_clear 0x411530 */
#include "char_integrate.h"   /* body_action_reset 0x426f20 */
#include "behave_helpers.h"
#include "engine_calls.h"
#include "entity_move.h"      /* entity_move_vertical 0x54e5c0 */
#include "npc_ai.h"           /* npc_navigate 0x43f880, grid_c_query 0x43ee30 */

_Static_assert(sizeof(path_rec) == 20, "path step 20 B");
#if UINTPTR_MAX == 0xFFFFFFFFu   /* (the retail layout: pinned on 32-bit, like world_layout.h) */
_Static_assert(offsetof(path_finder, tx) == 0xa004, "pf +0xa004");
_Static_assert(offsetof(path_finder, len) == 0xa020, "pf +0xa020");
_Static_assert(offsetof(path_finder, rec) == 0xa024, "pf +0xa024");
_Static_assert(offsetof(path_finder, wcells) == 0xa524, "pf +0xa524");
_Static_assert(sizeof(path_finder) == 0xa52c, "pf is 0xa52c bytes");
#endif

static int32_t  ld32(const void *p, uint32_t off) { int32_t v; memcpy(&v, (const uint8_t *)p + off, 4); return v; }
static uint16_t ld16(const void *p, uint32_t off) { uint16_t v; memcpy(&v, (const uint8_t *)p + off, 2); return v; }
static void st32(void *p, uint32_t off, int32_t v) { memcpy((uint8_t *)p + off, &v, 4); }
static void st16(void *p, uint32_t off, uint16_t v) { memcpy((uint8_t *)p + off, &v, 2); }
#define IP(p) ((intptr_t)(p))

/* the grid's region D (u16 per cell, col * 128 + row): 1 / 2 / 3 slopes and ladders, 0 open */
static uint16_t cell_d(const uint8_t *g, int32_t i) { return ld16(g, (uint32_t)(0x2c1040 + 2 * i)); }

int body_near_x(const wl_body *b, int32_t x)
{
    if (b->w <= 4000) return abs(x - b->x) < 1600;
    return b->w / 2 + 1600 >= abs(x - b->x);
}

int cast_face_cmd(const wl_body *b, int32_t dir, wl_cmd *cmd)
{
    if (dir == 99) return 1;
    if (b->facing == dir) return 1;
    cmd->lr = 2 - (dir != 1);
    return 0;
}

int path_dfs(path_finder *pf, int32_t c, int32_t r, int32_t depth, uint16_t *len, int32_t cost, int32_t bound,
             uint16_t *cost_out)
{
    uint16_t sub = 0, clen;        /* (retail: a stack dword / the bound's argument slot reused) */
    int32_t lim = bound;
    int found = 0;
    if ((uint16_t)cost > (uint16_t)bound) return 0;
    if (c < 0) return 0;
    const uint8_t *g = pf->grid;
    if (c >= ld32(g, 0x2c1030) || r < 1 || r >= ld32(g, 0x2c1034) - 1) return 0;
    int32_t cb = c << 7, idx = cb + r;
    const uint8_t *link = g + 0x1d1030 + 48 * idx;   /* region E: u16 count, +4 kind, +8 (col, row) pairs */
    int blocked = 0;
    if (cell_d(g, idx - 1) == 1 || cell_d(g, idx) == 1 || cell_d(g, idx + 1) == 0) {
        if (ld16(link, 0) == 0) return 0;
        blocked = 1;
    }
    if (pf->check_c != 0) {
        int32_t out = 0;
        if (grid_c_query(g, 3200 * c, 3200 * (r + 1) - pf->h, &out) != 0 && out != 0 && out != 2) return 0;
    }
    int32_t vi = c + 160 * r;
    uint16_t seen = pf->seen[vi];
    if (seen != 0 && seen < (uint16_t)cost) return 0;
    int32_t d = abs(r - pf->ty) + abs(c - pf->tx);
    if (pf->best < 0 || d < pf->best) {
        pf->best_r = r;
        pf->best_c = c;
        pf->best = d;
    }
    int goal = !(pf->links != 0 && ld16(link, 0) != 0 && ld32(link, 4) == 1);
    if (goal && pf->tx == c && pf->ty == r) {
        *len = (uint16_t)depth;
        *cost_out = (uint16_t)cost;
        return 1;
    }
    pf->seen[vi] = (uint16_t)cost;
    path_rec best = { 0 };
    if (!blocked) {
        /* the two horizontal neighbours, toward the goal first; a slope / ladder cell moves the row */
        int32_t dirs[2] = { 1, -1 };
        if (pf->tx <= c) { dirs[0] = -1; dirs[1] = 1; }
        for (int k = 0; k < 2; k++) {
            int32_t nc = c + dirs[k], nr = r;
            if (dirs[k] > 0) {
                if (nc < ld32(g, 0x2c1030)) {
                    if (r > 0 && cell_d(g, (nc << 7) + r) == 2) nr = r - 1;
                    if (nr < ld32(g, 0x2c1034) - 1 && cell_d(g, cb + nr + 1) == 3) nr++;
                }
            } else if (nc >= 0) {
                if (r > 0 && cell_d(g, (nc << 7) + r) == 3) nr = r - 1;
                if (nr < ld32(g, 0x2c1034) - 1 && cell_d(g, cb + nr + 1) == 2) nr++;
            }
            if (path_dfs(pf, nc, nr, depth + 1, &clen, cost + 1, lim, &sub)) {
                found = 1;
                best.kind = 0;
                best.nc = nc; best.nr = nr; best.c = c; best.r = r;
                lim = (int32_t)sub - 1;
                *len = clen;
                *cost_out = sub;
            }
        }
    }
    if (pf->links != 0 && ld16(link, 0) != 0) {
        for (uint16_t k = 0; k < ld16(link, 0); k++) {   /* a link cell's jumps cost 4 more */
            int32_t lc = ld32(link, 8u + 8u * k), lr = ld32(link, 0xcu + 8u * k);
            if (path_dfs(pf, lc, lr, depth + 1, &clen, cost + 4, lim, &sub)) {
                best.nc = lc; best.nr = lr; best.c = c; best.r = r;
                lim = (int32_t)sub - 1;
                *len = clen;
                *cost_out = sub;
                found = 1;
                best.kind = 1;
            }
        }
    }
    if (found) {
        path_rec *o = &pf->rec[(uint16_t)depth];
        o->kind = best.kind;
        o->nc = best.nc; o->nr = best.nr; o->c = best.c; o->r = best.r;
    }
    return found;
}

/* the body's start cell: its feet row, stepped off a slope / ladder top; `c` the column (moved right by the
 * body's width when the cell above the feet is open there but not here) */
static void start_cell(const path_finder *pf, const uint8_t *g, int32_t c0, int32_t r0, int32_t *pc, int32_t *pr)
{
    int32_t cb = c0 << 7, c = c0, r = r0;
    uint16_t v = cell_d(g, cb + r0);
    if (v > 0 && v <= 3 && cell_d(g, cb + r0 - 1) != 1) r = r0 - 1;
    uint16_t up = cell_d(g, cb + r + 1);
    if (up == 0 && cell_d(g, ((c0 + pf->wcells) << 7) + r + 1) != 0) {
        c = c0 + pf->wcells;
    } else if (cell_d(g, cb + r) != 0 && up == 0) {
        r--;
    }
    v = cell_d(g, (c << 7) + r);
    if (v >= 2 && v <= 3) r--;
    *pc = c;
    *pr = r;
}

int path_search(path_finder *pf, int32_t x, int32_t y, int32_t w, int32_t h, int32_t tx, int32_t ty, int32_t links,
                int32_t maxd, uint8_t *grid, uint16_t *count, path_rec *out, int32_t check_c)
{
    pf->links = links;
    pf->grid = grid;
    int32_t col = x / 3200;
    if (col < 0) col = 0;
    int32_t c0 = ld32(grid, 0x2c1030) - 1 < col ? ld32(grid, 0x2c1030) - 1 : col;
    pf->h = h;
    int32_t r0 = (y + h - 1) / 3200;
    pf->wcells = (w + 3199) / 3200;
    memset(pf->seen, 0, sizeof pf->seen);
    pf->tx = tx / 3200;
    pf->best = -1;
    pf->check_c = check_c;
    pf->ty = ty / 3200;
    int32_t c, r, out_c;
    uint16_t cost;
    start_cell(pf, grid, c0, r0, &c, &r);
    if (check_c != 0 && grid_c_query(grid, 3200 * c, 3200 * (r + 1) - pf->h, &out_c) != 0 && out_c != 0 && out_c != 2)
        pf->check_c = 0;
    int res = path_dfs(pf, c, r, 0, &pf->len, 1, 64, &cost);
    if (!(maxd != -1 && pf->best > maxd) && res == 0) {
        /* no path: search for the closest cell reached instead */
        int32_t bc = pf->best_c, br = pf->best_r;
        memset(pf->seen, 0, sizeof pf->seen);
        pf->ty = br;
        pf->check_c = check_c;
        pf->tx = bc;
        pf->best = -1;
        start_cell(pf, grid, c0, r0, &c, &r);
        if (check_c != 0 && grid_c_query(grid, 3200 * c, 3200 * (r + 1) - pf->h, &out_c) != 0 && out_c != 0 &&
            out_c != 2)
            pf->check_c = 0;
        res = path_dfs(pf, c, r, 0, &pf->len, 1, 64, &cost);
    }
    if (res == 0) return 0;
    /* the steps out, consecutive walks in one direction merged */
    *count = 0;
    for (int32_t i = 0; i < pf->len; i++) {
        const path_rec *p = &pf->rec[i];
        if (p->kind == 0 && i < pf->len - 1 && pf->rec[i + 1].kind == 0 &&
            (p->nc > p->c) == (pf->rec[i + 1].nc > pf->rec[i + 1].c))
            continue;
        path_rec *o = &out[*count];
        o->kind = p->kind;
        o->nc = p->nc; o->nr = p->nr; o->c = p->c; o->r = p->r;
        (*count)++;
    }
    return res;
}

/* the cast's teleport: the body's feet at (x, y), at rest, its water / region-C flags re-read */
static void cast_place(wl_char *ch, wl_body *b, int32_t x, int32_t y)
{
    b->x = x;
    b->y = y - b->h;
    b->vel_x = 0;
    b->vvel = 0;
    b->c_flag = grid_c_query(g_world->grid, b->x + b->w / 2, b->y + b->h - 1, NULL);
    b->slow = grid_c_query(g_world->grid, b->x + b->w / 2, b->y + b->margin, NULL);
    b->state = 0;
    b->sub = 0;
    b->ctr = 0;
    (void)ch;
    body_action_reset(b);
}

/* the cast's walk-off: every AI field the navigator uses cleared (the scene owns the character) */
static void cast_ai_clear(uint8_t *a)
{
    st16(a, 0x1429c, 0); st16(a, 0x147a0, 0); st16(a, 0x1422c, 0); st16(a, 0x1422e, 0); st16(a, 0x14230, 0);
    st16(a, 0x14278, 0); st16(a, 0x147ac, 0); st16(a, 0x147f4, 0); st16(a, 0x14232, 0); st16(a, 0x14802, 0);
    st32(a, 0x14270, 0); st32(a, 0x15b8c, 0);
    st32(a, 0x1427c, 0); st16(a, 0x14234, 0); st16(a, 0x14236, 0); st16(a, 0x14804, 0); st16(a, 0x14288, 0);
    st32(a, 0x1428c, 0); st16(a, 0x14248, 0); st16(a, 0x14290, 0); st32(a, 0x14294, 0); st32(a, 0x1423c, 0);
    st32(a, 0x14810, 0); st32(a, 0x14814, 0);
}

int cast_step(wl_char *c)
{
    uint8_t *a = (uint8_t *)c;
    wl_body *b = c->base.body;
    wl_cmd *cmd = &c->cmd;
    int32_t gy;
    int32_t loose = entity_move_vertical(&c->base, b, 1, b->drop, 0, &gy, NULL) == 0;
    cmd_clear(cmd);
    int done = 0, late = 0;
    int32_t dur = ld32(a, 0x15b88);
    if (dur > 0) {
        if (ld32(a, 0x15b8c) < dur) st32(a, 0x15b8c, ld32(a, 0x15b8c) + 1);
        else late = 1;
    }
    switch (c->ai_state) {
    case 1: beh_441f50((wl_char *)(c), (int32_t *)(IP(b)), loose); return 0;
    case 7: g_eng.h441de0(c, IP(b), loose); return 0;
    default: break;
    }
    int32_t mm = c->move_mode;
    int ground = 0;   /* (retail: unset for a move mode outside 0..3 — none exists) */
    if (mm == 0) ground = 1;
    else if (mm > 0 && mm <= 3) ground = 0;
    switch (ld32(a, 0x15b64)) {
    case 3:   /* wait for the action to end (a long one is cut) */
        if (b->pose != 0) {
            if (b->act_len / 100 <= 1) {
                b->pose = 0; b->act_len = 210; b->f58 = 0; b->act_ctr = 0; b->act_78 = 0; b->act_arg = 0;
                b->spell = 0; b->target = 0; b->act_5a = 0; b->act_64 = 0;
                return 0;
            }
            return 0;
        }
        if (ground == 0) goto fin_done;
        if (loose == 0) return 0;
        done = 1;
        goto fin;
    case 2: {   /* face */
        int32_t dir = ld32(a, 0x15b74);
        if (dir == 99 || b->facing == dir) goto fin_done;
        cmd->lr = dir != 1 ? 1 : 2;
        return 0;
    }
    case 5:   /* the pose skill, repeated */
        switch (ld16(a, 0x15b68)) {
        case 0:
            if (!(b->pose == 0 && (loose != 0 || ld32(a, 0x15b9c) == 0)) && late == 0 && mm == 0) return 0;
            cmd->act = 14;
            cmd->skill = (uint16_t)ld32(a, 0x15b98);
            cmd->skill_kind = 210;
            cmd->skill_arg = (uint16_t)ld32(a, 0x15ba4);
            cmd->skill_ptr = ld32(a, 0x15ba0);
            st16(a, 0x15b68, (uint16_t)(ld16(a, 0x15b68) + 1));
            return 0;
        case 1:
            cmd->act = 14;
            if (b->f66 < 1000) return 0;
            st16(a, 0x15b6a, (uint16_t)(ld16(a, 0x15b6a) + 1));
            if ((int32_t)ld16(a, 0x15b6a) <= ld32(a, 0x15b94)) return 0;
            st16(a, 0x15b68, (uint16_t)(ld16(a, 0x15b68) + 1));
            return 0;
        case 2:
            if (b->pose != 0 || b->f66 != 0) return 0;
            done = 1;
            goto fin;
        default:
            return 0;
        }
    case 4:   /* the stance skill */
        if (ld16(a, 0x15b68) != 0) {
            if (b->pose != 0) return 0;
            done = 1;
            goto fin;
        }
        if (!(b->pose == 0 && (loose != 0 || ld32(a, 0x15b9c) == 0)) && late == 0 && mm == 0) return 0;
        cmd->skill_kind = 900;
        cmd->skill = (uint16_t)ld32(a, 0x15b98);
        cmd->skill_arg = (uint16_t)ld32(a, 0x15ba4);
        cmd->skill_ptr = ld32(a, 0x15ba0);
        st16(a, 0x15b68, (uint16_t)(ld16(a, 0x15b68) + 1));
        return 0;
    case 1:   /* walk to (+0x15b6c, +0x15b70) */
        break;
    default:
        return 0;
    }

    if (late != 0) {   /* out of time: teleport there (once), then face and end */
        if (ld32(a, 0x15b90) != 0) {
            b->facing = ld32(a, 0x15b74);
            done = 1;
            goto fin;
        }
        cast_ai_clear(a);
        st32(a, 0x15b90, ld32(a, 0x15b90) + 1);
        if (ld32(a, 0x15b7c) == 0) cast_place(c, b, ld32(a, 0x15b6c), ld32(a, 0x15b70));
        else cast_place(c, b, ld32(a, 0x15b80), ld32(a, 0x15b84));
    }
    if (ground == 0) {   /* swimming / flying: straight there */
        if (beh_442640((wl_char *)(c), (int32_t *)(IP(b)), loose, (int32_t)(ld32(a, 0x15b6c)), (int32_t)(ld32(a, 0x15b70)), (int32_t)(ld32(a, 0x15b74))) == 0) return 0;
        goto fin_done;
    }
    switch (ld16(a, 0x15b68)) {
    case 0:
    case 2:
        if (loose == 0) return 0;
        st16(a, 0x15b68, (uint16_t)(ld16(a, 0x15b68) + 1));
        /* fall through */
    case 1:
    case 3:
        if (ld16(a, 0x147a0) < ld16(a, 0x1429c)) break;
        {   /* plan the path */
            int32_t tx = ld32(a, 0x15b6c), ty = ld32(a, 0x15b70);
            int32_t nav_c = c->nav_c_check;
            st16(a, 0x1429c, 0);
            st16(a, 0x147a0, 0);
            st32(a, 0x147a4, tx);
            st32(a, 0x147a8, ty);
            int32_t chk = nav_c == 0 ? 0 : c->fc7ec != 0 ? 0 : c->party->f45e == 0;
            if (path_search((path_finder *)g_world->f1050, b->x, b->y, b->w, b->h, tx, ty, 1, 2, g_world->grid,
                            (uint16_t *)(a + 0x1429c), (path_rec *)(a + 0x142a0), chk) != 0)
                break;
            /* no path */
            if (ld16(a, 0x15b68) == 1) {
                const uint8_t *p = (const uint8_t *)b->attach;
                if (p != NULL) {
                    int32_t k = ld32(p, 0x1c0);
                    if ((k == 0 || k == 1) ? ld32(a, 0x14238) != (int32_t)(intptr_t)p : k == 3) {
                        /* riding a platform: walk off it first */
                        c->ai_state = 7;
                        c->ai_sub = 0;
                        c->ai_t = 0;
                        c->walk_x = tx;
                        st16(a, 0x15b68, 2);
                        return done;
                    }
                }
            }
            cast_place(c, b, ld32(a, 0x15b80), ld32(a, 0x15b84));
            if (ld32(a, 0x15b90) == 0) {
                st32(a, 0x15b90, 1);
                st32(a, 0x15b8c, 0);
            } else {
                b->facing = ld32(a, 0x15b74);
                done = 1;
            }
        }
        break;
    default:
        return 0;
    }

    /* follow the path */
    int32_t no_run = ld32(a, 0x15b78), face = ld32(a, 0x15b74), tx = ld32(a, 0x15b6c);
    uint16_t idx = ld16(a, 0x147a0), n = ld16(a, 0x1429c);
    if (idx < n) {
        const path_rec *s = (const path_rec *)(a + 0x142a0) + idx;
        if (s->kind == 1) {
            beh_442710((wl_char *)(c), (uint16_t)(idx));
            st16(a, 0x1429c, 0);
            st16(a, 0x147a0, 0);
            if (done == 0) return 0;
            goto fin;
        }
        if (s->kind == 0) {
            if (idx == 0 && n == 1 && abs(ld32(a, 0x147a4) - b->x) <= 1600) {
                st16(a, 0x147a0, 1);
            } else {
                int32_t dest = idx == n - 1 ? tx : s->nc * 3200;
                if (body_near_x(b, dest)) st16(a, 0x147a0, (uint16_t)(idx + 1));
                else npc_navigate(c, dest, 1600, b, loose, 0, 0, 0, no_run, 0);
            }
        }
    }
    if (ld16(a, 0x147a0) < ld16(a, 0x1429c)) goto check;
    /* the last step: arrive, then turn */
    if (npc_navigate(c, tx, 800, b, loose, 0, 0, 0, no_run, 0) == 0) goto check;
    if (face != 99) {
        if (cmd->lr != 0) goto check;
        if (!cast_face_cmd(b, face, cmd)) goto check;
    }
    if (b->vel_x >= 10000) goto check;
    done = 1;
    goto fin;
check:
    if (done == 0) return 0;
    goto fin;
fin_done:
    done = 1;
fin:
    cmd_clear(cmd);
    return done;
}
