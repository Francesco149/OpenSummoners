/*
 * character.c — see character.h.  The controllable-character WALK reduction:
 * the held-axis -> command intent (0x478ba0) and the horizontal velocity-ramp
 * apply (0x442a70 case 0x75 -> the clamp-ramp 0x445db0 -> the worldX commit
 * 0x54db10), reduced to the open-air flat walk and fit BIT-EXACT to Arche's
 * ground-truth per-tick body (runs/mover-caller, ckpt 114).  Pure C, Win32-free.
 */
#include "character.h"

void character_init(character *c, int32_t spawn_world_x, int facing)
{
    if (c == NULL) return;
    c->world_x  = spawn_world_x;
    c->vel      = 0;
    c->facing   = (int16_t)((facing == CHAR_FACE_LEFT) ? CHAR_FACE_LEFT
                                                       : CHAR_FACE_RIGHT);
    c->cmd_dir  = 0;
    c->warm     = 0;
    c->held_dir = 0;
}

/* Ramp cur toward target by at most |rate| (the open-air reduction of the
 * 0x445db0 clamp-ramp: it adds/subtracts the rate and clamps at the target). */
static int32_t ramp_toward(int32_t cur, int32_t target, int32_t rate)
{
    if (cur < target)      { cur += rate; if (cur > target) cur = target; }
    else if (cur > target) { cur -= rate; if (cur < target) cur = target; }
    return cur;
}

int32_t character_step(character *c, const int *axis_held)
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
    return dwx;
}
