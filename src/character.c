/*
 * character.c — see character.h.  The controllable-character WALK reduction:
 * the held-axis -> command intent (0x478ba0) and the horizontal velocity-ramp
 * apply (0x442a70 case 0x75 -> the clamp-ramp 0x445db0 -> the worldX commit
 * 0x54db10), reduced to the open-air flat walk and fit BIT-EXACT to Arche's
 * ground-truth per-tick body (runs/mover-caller, ckpt 114).  Pure C, Win32-free.
 */
#include "character.h"

void character_init(character *c, int32_t spawn_world_x, int32_t spawn_world_y,
                    int facing)
{
    if (c == NULL) return;
    c->world_x   = spawn_world_x;
    c->world_y   = spawn_world_y;
    c->vel       = 0;
    c->vvel      = 0;
    c->ground_y  = spawn_world_y;
    c->facing    = (int16_t)((facing == CHAR_FACE_LEFT) ? CHAR_FACE_LEFT
                                                        : CHAR_FACE_RIGHT);
    c->cmd_dir   = 0;
    c->warm      = 0;
    c->held_dir  = 0;
    c->airborne  = 0;
    c->jump_held = 0;
}

/* Ramp cur toward target by at most |rate| (the open-air reduction of the
 * 0x445db0 clamp-ramp: it adds/subtracts the rate and clamps at the target). */
static int32_t ramp_toward(int32_t cur, int32_t target, int32_t rate)
{
    if (cur < target)      { cur += rate; if (cur > target) cur = target; }
    else if (cur > target) { cur -= rate; if (cur < target) cur = target; }
    return cur;
}

int32_t character_step(character *c, const int *axis_held, int jump_held)
{
    if (c == NULL) return 0;

    /* ── AI reduction (0x478ba0): held axis -> commanded direction ────────────
     * LEFT -> -1 (cmd[0]=1 walk), RIGHT -> +1 (cmd[0]=2 walk).  Both/neither
     * held = no net direction.  UP/DOWN (cmd[3]=0xb/10) are read by the real AI
     * but unused until vertical movement (ladders/jump) lands — PORT-DEBT. */
    int want = 0;
    if (axis_held != NULL) {
        int l = axis_held[CHAR_AXIS_LEFT]  != 0;
        int r = axis_held[CHAR_AXIS_RIGHT] != 0;
        if (l && !r)      want = -1;
        else if (r && !l) want = +1;
    }

    /* The press->latch warmup (PORT-DEBT(char-input-autorepeat)).  A fresh press
     * or a change of held direction re-arms the counter; the walk command latches
     * only after CHAR_INPUT_REPEAT_DELAY consecutive held ticks (reproducing the
     * capture's 2 idle ticks before motion), then self-sustains while held. */
    if (want != c->held_dir) c->warm = 0;
    c->held_dir = (int16_t)want;

    if (want == 0) {
        c->cmd_dir = 0;                       /* released */
    } else {
        if (c->warm < CHAR_INPUT_REPEAT_DELAY)
            c->warm = (int16_t)(c->warm + 1);
        c->cmd_dir = (c->warm >= CHAR_INPUT_REPEAT_DELAY) ? (int16_t)want : 0;
    }

    /* ── APPLY reduction (0x442a70 case 0x75): ramp the horizontal velocity, flip
     *    facing at rest, commit worldX += vel/100 (flat reduction of 0x54db10). ─
     * vel is the signed accumulator body+0x28 (+ right, - left). */
    if (c->cmd_dir != 0) {
        int want_face = (c->cmd_dir > 0) ? CHAR_FACE_RIGHT : CHAR_FACE_LEFT;
        if (c->facing == want_face) {
            /* facing matches the move dir -> accelerate toward the cap */
            int32_t target = (int32_t)c->cmd_dir * CHAR_WALK_CAP;
            c->vel = ramp_toward(c->vel, target, CHAR_WALK_ACCEL);
        } else if (c->vel == 0) {
            /* at rest, commanded the other way -> flip facing now (accel next tick,
             * the integrator's local_14 v==0 facing flip) */
            c->facing = (int16_t)want_face;
        } else {
            /* still moving the wrong way -> decelerate toward 0 first.  The
             * decompile uses -accel here; the capture has an idle gap (no direct
             * reverse) so the rate is PORT-DEBT(char-collision-mover). */
            c->vel = ramp_toward(c->vel, 0, CHAR_WALK_BRAKE);
        }
    } else {
        /* idle / released / warming -> brake toward 0; facing holds */
        c->vel = ramp_toward(c->vel, 0, CHAR_WALK_BRAKE);
    }

    int32_t dwx = c->vel / 100;               /* the 0x54db10 step (flat) */
    c->world_x += dwx;

    /* ── VERTICAL jump integrator (0x442a70 case 3, the body+0x38==3 airborne
     *    sub-FSM): launch impulse -> asymmetric gravity -> ground clamp. ───────
     * worldY += vvel/100 each airborne tick (the 0x54e5c0 collision mover, here
     * reduced to a flat ground clamp at ground_y — PORT-DEBT(char-collision-mover)
     * for the real terrain surface).  Bit-exact to the captured arc (runs/runjump-
     * gt, ckpt 116; tests/test_character.c). */
    int jump_edge = (jump_held != 0) && (c->jump_held == 0);
    c->jump_held = (int16_t)(jump_held != 0);

    if (!c->airborne) {
        if (jump_edge) {
            /* Launch (windup-complete tick).  vvel := impulse, step worldY by the
             * pre-grav impulse, then add one FALL grav step (the grav was selected
             * from the pre-impulse vvel==0 = fall branch) -> first sampled -76000. */
            c->airborne = 1;
            c->vvel = CHAR_JUMP_IMPULSE;
            c->world_y += c->vvel / 100;
            c->vvel += CHAR_JUMP_FALL_GRAV;
        }
        /* else grounded: world_y holds at ground_y, vvel 0. */
    } else {
        int32_t step = c->vvel / 100;
        if (c->vvel > 0 && c->world_y + step > c->ground_y) {
            /* a downward step that would penetrate the ground -> the collision
             * mover returns blocked: clamp to the surface and zero vvel (line
             * 926-931 -> 0x426f50 grounded). */
            c->world_y = c->ground_y;
            c->vvel = 0;
            c->airborne = 0;
        } else if (c->vvel < 0) {
            /* RISING: variable-height decel — jump HELD floats up slowly (2000),
             * RELEASED cuts the rise short (8000); release mid-rise to shorten it. */
            c->world_y += step;
            c->vvel += jump_held ? CHAR_JUMP_RISE_GRAV_HELD : CHAR_JUMP_RISE_GRAV_FREE;
        } else {
            /* FALLING: the (button-independent) fall accel. */
            c->world_y += step;
            c->vvel += CHAR_JUMP_FALL_GRAV;
        }
    }

    return dwx;
}
