/* char_physics.c — FUN_00467bb0 / FUN_004385e0.  See char_physics.h.
 *
 * Transcribed from the decompile; labels keep its control flow.  Two fidelity notes:
 *   - move_mode outside 0..3 leaves retail's "can jump" flag holding the BODY POINTER
 *     and three other flags uninitialized (stack garbage); the port zeroes those
 *     three (modes 0..3 are the only ones the shipped tuning uses).
 *   - mode 3 reads the bounds against `self`'s REAL part (entity+0x40), not `b`. */
#include "char_physics.h"

#include <stddef.h>
#include <string.h>

#include "entity_move.h"
#include "map_grid.h"

#define BODY(k) (&self->base.body[(uint16_t)(k) < self->base.nbody ? (uint16_t)(k) : 0])

static int32_t grid_i32(size_t off) { int32_t v; memcpy(&v, g_world->grid + off, 4); return v; }
static uint16_t grid_u16(size_t off) { uint16_t v; memcpy(&v, g_world->grid + off, 2); return v; }

int32_t char_part_top(const wl_char *self, uint16_t part)
{
    const wl_body *p = &self->base.body[part < self->base.nbody ? part : 0];
    return p->y + p->margin;
}

static void set_state(wl_body *b, uint16_t st) { b->state = st; b->sub = 0; b->ctr = 0; }

int char_physics_step(wl_char *self, const wl_cmd *cmd, wl_body *b, int32_t probe,
                      int32_t ctl, int32_t part, int32_t grounded)
{
    int32_t grav = self->gravity, cap = self->fall_cap;
    int block_air = 0;       /* v84 */
    int floating = 0;        /* v78 */
    int32_t can_up;          /* v9 (a3) */
    int32_t jump_ok = 0;     /* v74 */
    int32_t on_ground = 0;   /* v76 */
    int to_air = 0;          /* v80 */
    if (b->slow != 0) cap = self->fall_cap / 3;

    int32_t mm = self->move_mode;
    if (mm != 0) {
        if (mm <= 0 || mm > 3) {
            can_up = (int32_t)(uintptr_t)b;
        } else {
            switch (b->state) {
            case 0: case 1: case 4: can_up = 1; break;
            case 2:                 can_up = b->sub == 2; break;
            default:                can_up = 0; break;
            }
            on_ground = can_up; jump_ok = can_up; to_air = 0;
            b->drop = 1;
            block_air = 1;
        }
    } else {
        switch (b->state) {
        case 0: case 1: case 4: can_up = 1; break;
        case 2:                 can_up = b->sub == 2; break;
        default:                can_up = 0; break;
        }
        jump_ok = can_up && grounded != 0;
        on_ground = can_up && grounded != 0;
        if (probe == 0 && grounded != 0) self->air_jumps = 0;
        if (self->air_jumps < self->max_air_jumps) jump_ok = 1;
        to_air = 1;
        if (cmd->jump == 9) b->drop = 8;
        else if (b->drop != 0) b->drop = (uint8_t)(b->drop - 1);
    }

    /* the float boost: a gap of float_h or more between the feet and the next region-D
     * floor below (either side column) */
    if (self->float_h > 0) {
        int32_t dim0 = grid_i32(MG_DIM0), dim1 = grid_i32(MG_DIM1);
        int32_t c0 = b->x / 3200; if (c0 < 0) c0 = 0; if (dim0 - 1 < c0) c0 = dim0 - 1;
        int32_t c1 = (b->w + b->x - 1) / 3200; if (c1 < 0) c1 = 0; if (dim0 - 1 < c1) c1 = dim0 - 1;
        int32_t i;
        for (i = (b->y + b->h - 1) / 3200 + 1; i < dim1; i++)
            if (grid_u16(MG_REGION_D + 2u * (uint32_t)(i + (c0 << 7))) != 0 ||
                grid_u16(MG_REGION_D + 2u * (uint32_t)(i + (c1 << 7))) != 0)
                break;
        if (self->float_h <= 3200 * i - b->y - b->h) {
            if (cap < 50000) cap = 50000;
            if (grav < 2000) grav = 2000;
            floating = 1;
        }
    }
    if (b->fall_cap != 0) { cap = b->fall_cap; grav = b->fall_cap / 10; }

    switch (ctl) {
    case 0: break;
    case 1:
        if (b->state != 3) set_state(b, 0);
        can_up = 0; jump_ok = 0; on_ground = 0;
        break;
    case 2: case 3:
        set_state(b, 0);
        can_up = 0; jump_ok = 0; on_ground = 0;
        break;
    default:
        can_up = 0; on_ground = 0; jump_ok = 0;
        break;
    }

    /* DOWN: crouch (grounded) / slide (mode 1..3, airborne) */
    if (cmd->ud != 10) goto down_done;
    if (on_ground != 0 && self->can_crouch != 0) { set_state(b, 2); goto down_done; }
    if (block_air == 0) goto down_done;
    if (grounded == 0) { set_state(b, 6); goto no_up; }
    goto up_check;
down_done:
    if (grounded != 0) goto up_check;
no_up:
    if (self->f15a24 == 0) can_up = 0;
up_check:
    if (cmd->ud == 11 && can_up != 0) {
        if (self->up_assist != 0) set_state(b, 5);
    }
    if (self->f15a24 != 0 && b->state == 5) to_air = 0;
    if (cmd->jump == 7 && (jump_ok != 0 || (b->state == 5 && grounded != 0)) && self->can_jump != 0 && floating == 0) {
        set_state(b, 3);
        if (probe == 0 && self->air_jumps < self->max_air_jumps && grounded == 0) self->air_jumps++;
    }

    int ok;   /* v25 */
    switch (b->state) {
    case 2:
        if (b->sub != 0) {
            if (b->sub != 1) { if (b->sub - 1 == 1) goto sub2_tick; goto vert_check; }
            b->ctr++;
            b->margin = self->crouch_margin;
            if (cmd->ud != 10 && b->ctr > 4u) { b->sub = 2; b->ctr = 0; }
        } else if (++b->ctr > 4u) {
            b->sub = 1; b->ctr = 0;
        }
        goto vert_check;
    case 3:
        switch (b->sub) {
        case 0:
            if (++b->ctr > 4u) { b->sub = 1; b->vvel = self->jump_impulse; b->ctr = 0; }
            goto vert_check;
        case 1:
            b->sub = 2;
            goto rise;
        case 2:
            if (b->vvel > -20000) b->sub = 3;
            goto rise;
        case 3:
            b->ctr++;
rise:
            if (b->vvel >= 0) { ok = b->ctr <= 6u; goto state_end; }
            grav = cmd->jump != 0 ? self->rise_grav_held : self->rise_grav_free;
            if (floating == 1) grav = self->rise_grav_free;
            goto vert_check;
        default:
            goto vert_check;
        }
    case 4:
        if (b->sub != 0) {
            if (b->sub == 1) b->sub = 2;
            else if (b->sub - 1 == 1) goto sub2_tick;
        } else {
            if (grounded == 0) goto vert_move;
            b->sub = 1;
        }
        goto vert_check;
    case 5:
        if (b->sub != 0) { if (b->sub == 1) goto sub2_tick; goto vert_check; }
        b->ctr++;
        if (cmd->ud != 11 && b->ctr > 16u) { b->sub = 1; b->ctr = 0; }
        goto vert_check;
    case 6:
        grav = self->slide_grav; cap = self->slide_cap;
        if (b->ctr >= 8u) goto state_reset;
        b->ctr++;
        goto vert_check;
    default:
        goto vert_check;
    }
sub2_tick:
    ok = ++b->ctr <= 4u;
state_end:
    if (!ok) {
state_reset:
        set_state(b, 0);
    }
vert_check:
    if (grounded != 0 && b->vvel >= 0) { b->vvel = 0; goto horizontal; }
vert_move:
    mm = self->move_mode;
    if (mm < 0) goto air;
    if (mm > 2) {
        if (mm != 3) goto air;
        int32_t dy = b->vvel / 100;
        if (dy < 0) {
            int32_t lim = -self->bound_top;
            const wl_body *rb = BODY(part);
            if (lim > dy + rb->y + rb->margin) {
                if (lim - rb->margin - rb->y > 0) goto air;
                dy = lim - char_part_top(self, (uint16_t)part);
            }
        } else {
            int32_t lim = self->bound_bottom + grid_i32(MG_DIM1_PX);
            const wl_body *rb = BODY(part);
            if (lim < dy + rb->y + rb->h - 1) {
                if (lim - rb->h - rb->y + 1 >= 0) b->y += lim - rb->h - rb->y + 1;
                goto air;
            }
        }
        b->y += dy;
        goto air;
    }
    if (entity_move_vertical(&self->base, b, b->vvel / 100, b->drop, (uint16_t)part, &b->y,
                             (wl_entity **)&b->attach) == 0) {
        if (b->vvel > 0) grounded = 1;
        b->vvel = 0;
    }
air:
    if (to_air != 0 && (b->state < 3u || b->state > 4u)) set_state(b, 4);
    {
        int32_t v = b->vvel + grav;
        if (cap < v) v = cap;
        b->vvel = v;
    }

horizontal:;
    uint16_t st0 = b->state;      /* v44 */
    int steer = 1;                /* v45 */
    int32_t acc = self->brake;    /* v46 */
    int32_t vmax = self->walk_max;/* v86 */
    int32_t accel = self->walk_accel; /* v47 */
    int32_t brake = acc;          /* v77 */
    int allow_turn = 1;           /* v81 */
    int turn = 0, forward = 0, reverse = 0;   /* v73 v75 v79 */
    int32_t lr = 0;
    if (st0 == 2 || st0 == 5) steer = 0;
    if (ctl != 1) {
        if (ctl == 2) {
            if (grounded != 0) goto steer_done;
            allow_turn = 0;
        } else if (ctl == 3) {
            goto steer_done;
        }
        if (steer != 0) {
            if (grounded == 0 && self->air_max != 0) {
                lr = cmd->lr;
                switch (lr) {
                case 1: case 2: case 5: case 6:
                    if (self->can_jump != 0) {
                        if (b->vel_x >= vmax) { vmax = self->air_max; accel = self->air_accel_fast; }
                        else                  { vmax = self->air_max; accel = self->air_accel_slow; }
                    }
                    break;
                default:
                    break;
                }
            } else {
                lr = cmd->lr;
                if (lr >= 5 && lr <= 6 && self->can_run != 0) {
                    if (b->vel_x < vmax) accel = self->run_accel;
                    vmax = self->max_speed;
                }
            }
            if (self->boost_speed != 0) vmax += 6000;
            if (self->boost_accel != 0) { accel += 2000; brake = acc - 800; }
            switch (lr) {
            case 1: case 5:
                if (b->facing == 3)       { acc = accel; forward = 1; }
                else if (b->vel_x != 0)   { acc = -accel; reverse = 1; }
                else if (allow_turn != 0) turn = 1;
                break;
            case 2: case 6:
                if (b->facing == 1)       { acc = accel; forward = 1; }
                else if (b->vel_x != 0)   { acc = -accel; reverse = 1; }
                else if (allow_turn != 0) turn = 1;
                break;
            default:
                break;
            }
        }
    }
steer_done:
    /* the move state: 1 = moving (sub 1 = braking, up to 40 ticks) */
    if (b->state != 0) {
        if (st0 != 1) goto move_done;
        if (b->pose != 0) { b->state = 0; goto clear_sub; }
        if (b->sub != 0) {
            if (b->sub != 1) goto move_done;
            if (forward) { b->ctr = 0; b->sub = 0; goto move_done; }
            if (b->vel_x <= 0) { b->state = 0; goto clear_sub; }
            if (b->ctr < 40u) { b->ctr++; goto move_done; }
            goto stop;
        }
        if (b->vel_x < self->max_speed) b->ctr = 0;
        else if (b->ctr < 200u) b->ctr++;
        if (!forward) {
            if (b->ctr < 80u || !reverse) { b->ctr = 0; b->sub = 1; goto move_done; }
stop:
            b->state = 0;
            goto clear_sub;
        }
    } else if (forward && b->pose == 0) {
        b->state = 1;
clear_sub:
        b->sub = 0; b->ctr = 0;
    }
move_done:
    if (turn) {
        if (self->turn_anim != 0 && (grounded != 0 || self->turn_anim_air != 0)) {
            if (b->pose == 0) {
                b->pose = 2; b->act_len = 200; b->f58 = 0; b->act_ctr = 0; b->act_78 = 0;
                b->act_arg = 0; b->spell = 0; b->target = 0; b->act_5a = 0; b->act_64 = 0;
            }
        } else {
            b->facing = b->facing != 3 ? 3 : 1;
        }
    }

    int32_t v = b->vel_x, a = acc;
    if (vmax < 0) {
        if (acc > 0 || v > 0) {
            if (vmax >= v) { if (vmax > v) v = v - brake > 0 ? 0 : v - brake; }
            else { v -= acc; if (vmax > v) v = vmax; }
        } else if (acc < 0) {
            v = v - acc > 0 ? 0 : v - acc;
        }
    } else {
        if (v < 0 && acc < 0) a = -acc;
        if (a <= 0) { if (a < 0) v = v + a < 0 ? 0 : v + a; }
        else if (vmax <= v) { if (vmax < v) v = v + brake < 0 ? 0 : v + brake; }
        else { v += a; if (vmax < v) v = vmax; }
    }
    b->vel_x = v;

    if (self->move_mode != 3) {
        int32_t down = probe == 0 || self->f200 == 0;
        int32_t dx = b->facing == 3 ? v / -100 : v / 100;
        int r = entity_move_horizontal(&self->base, b, dx, down, 1, (int16_t)b->drop, (uint16_t)part, 0);
        if (r == 0) b->vel_x = 0;
        return r;
    }
    int32_t dx = b->facing == 1 ? v / 100 : v / -100;
    int32_t r;
    const wl_body *rb = BODY(part);
    if (dx < 0) {
        int32_t lim = -self->bound_left;
        r = dx + rb->x;
        if (lim > r) {
            if (lim - rb->x > 0) { r = b->x; b->x = r; return r; }
            r = 33 * ((uint16_t)part < self->base.nbody ? (uint16_t)part : 0);
            dx = lim - rb->x;
        }
        b->x += dx;
        return r;
    }
    r = self->bound_right + grid_i32(MG_DIM0_PX);
    if (r >= dx + rb->x + rb->w) { b->x += dx; return r; }
    if (r - rb->w - rb->x >= 0) { r = r - rb->w - rb->x + b->x; b->x = r; }
    else { r = b->x; b->x = r; }
    return r;
}
