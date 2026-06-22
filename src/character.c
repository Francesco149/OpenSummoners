/*
 * character.c — see character.h.  The controllable-character WALK reduction:
 * the held-axis -> command intent (0x478ba0) and the horizontal velocity-ramp
 * apply (0x442a70 case 0x75 -> the clamp-ramp 0x445db0 -> the worldX commit
 * 0x54db10), reduced to the open-air flat walk and fit BIT-EXACT to Arche's
 * ground-truth per-tick body (runs/mover-caller, ckpt 114).  Pure C, Win32-free.
 */
#include "character.h"
#include "input.h"     /* the event ring + input_dash_double_tap (dash trigger) */

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
    c->jump_sub  = 0;
    c->jump_ctr  = 0;
    c->cmd_lr    = 0;
    c->cmd_pose  = 0;
    c->sword_out = 0;
    c->attacking      = 0;
    c->attack_timer   = 0;
    c->attack_kind    = 0;
    c->attack_last_ms = 0;
}

/* Ramp cur toward target by at most |rate| (the open-air reduction of the
 * 0x445db0 clamp-ramp: it adds/subtracts the rate and clamps at the target). */
static int32_t ramp_toward(int32_t cur, int32_t target, int32_t rate)
{
    if (cur < target)      { cur += rate; if (cur > target) cur = target; }
    else if (cur > target) { cur -= rate; if (cur < target) cur = target; }
    return cur;
}

static int32_t iabs32(int32_t v) { return v < 0 ? -v : v; }

int32_t character_step(character *c, const int *axis_held, int jump_held, int run)
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
    if (c->attacking && !c->airborne) {
        /* SWORD SWING in progress (442a70's cmd[4] body state): movement is locked.
         * Brake vel to 0 at the WALK brake — the same lock the pose uses, holding her
         * in place through the swing — then the directional kinds layer a scripted
         * displacement on top (the registered attack templates 41f200:1181-1201):
         *   NEUTRAL/DOWN : stationary (DOWN ends in crouch; sword2.osr nets 0).
         *   FORWARD      : a LUNGE toward facing — a direct world_x step (the RE'd
         *                  movement mechanism 0x447ed0 -> the collision mover
         *                  0054db10, here the flat reduction), even-distributing the
         *                  captured net CHAR_ATTACK_FORWARD_LUNGE across the swing.
         *   BACK         : net 0; the facing flip is at completion (resolve_attack).
         * c->attack_timer is this tick's swing index (0..N-1 — resolve_attack advanced
         * it before character_step), so an exact-integer running sum lands the full
         * lunge over the N swing ticks with no rounding drift. */
        c->vel = ramp_toward(c->vel, 0, CHAR_WALK_BRAKE);
        if (c->attack_kind == CHAR_ATTACK_FORWARD) {
            int n    = character_attack_ticks(CHAR_ATTACK_FORWARD);
            int t    = c->attack_timer;
            int prev = (CHAR_ATTACK_FORWARD_LUNGE * t) / n;
            int curr = (CHAR_ATTACK_FORWARD_LUNGE * (t + 1)) / n;
            int sign = (c->facing == CHAR_FACE_LEFT) ? -1 : +1;
            c->world_x += sign * (curr - prev);
        }
    } else if (c->cmd_pose != 0 && !c->airborne) {
        /* CROUCH (cmd_pose 10) / UP-pose (0xb): the apply states 2/5 set bVar16=false
         * (0x442a70:959) -> SKIP the accel ramp -> brake the velocity toward 0 at the
         * WALK brake, even while a direction is STILL commanded.  That is "UP stops you
         * faster" (you decelerate without releasing the dash/walk) AND the crouch/slide
         * momentum bleed: a SLIDE is just a crouch (state 2) entered with momentum, a
         * CROUCH from rest just holds at 0.  Bit-exact -800/tick from any entry velocity
         * to 0 -- PROVEN from a REAL dash (runs/pose-demo/cap-slide3: cmd0=6 hvel 48000 ->
         * DOWN -> bstate 2, 48000->0 at -800/tick; cap-body: up-pose 24000->0).  On FLAT
         * ground (terrain [0x5653]=0, param_2!=0) DOWN is always state 2 -- the state-6
         * momentum slide ([0x5656/57]=64000/4000) needs terrain [0x5653] in [1,3] (a SLOPE),
         * unreached -> PORT-DEBT(char-slope-slide).  ckpt 154.
         * Facing holds (the pose does not flip it); the worldX commit below is unchanged. */
        c->vel = ramp_toward(c->vel, 0, CHAR_WALK_BRAKE);
    } else if (c->cmd_dir != 0) {
        int want_face = (c->cmd_dir > 0) ? CHAR_FACE_RIGHT : CHAR_FACE_LEFT;
        if (c->facing == want_face) {
            /* facing matches the move dir -> accelerate toward the cap.  RUN (dash)
             * raises the cap to 48000 and accelerates +3200/tick while |vel| < the
             * walk cap 24000, then the walk accel 1600 up to 48000 (the captured
             * two-phase ramp).  Releasing the dash while still holding the dir leaves
             * |vel| over the now-walk cap -> the 0x445db0 over-cap path decelerates at
             * the BRAKE rate toward the cap, then walks. */
            int32_t cap    = run ? CHAR_RUN_CAP : CHAR_WALK_CAP;
            int32_t target = (int32_t)c->cmd_dir * cap;
            if (iabs32(c->vel) > cap) {
                c->vel = ramp_toward(c->vel, target, CHAR_WALK_BRAKE);
            } else {
                int32_t accel = (run && iabs32(c->vel) < CHAR_WALK_CAP)
                                    ? CHAR_RUN_ACCEL : CHAR_WALK_ACCEL;
                c->vel = ramp_toward(c->vel, target, accel);
            }
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
     *    sub-FSM): a 4-tick launch WINDUP -> impulse -> asymmetric gravity ->
     *    ground clamp. ───────────────────────────────────────────────────────────
     * worldY += vvel/100 each airborne tick (the 0x54e5c0 collision mover, here
     * reduced to a flat ground clamp at ground_y — PORT-DEBT(char-collision-mover)
     * for the real terrain surface).  Bit-exact to the captured arc + windup
     * (runs/runjump-gt/capjump-ring2, ckpt 116/119; tests/test_character.c). */
    int jump_edge = (jump_held != 0) && (c->jump_held == 0);
    c->jump_held = (int16_t)(jump_held != 0);

    if (!c->airborne) {
        if (jump_edge) {
            /* Jump execute (cmd[2]==7 -> 0x426f50(body,3)): enter the airborne
             * state with sub-state 0 (windup) + a fresh counter.  The windup block
             * below runs THIS tick (the entry tick is windup tick 1). */
            c->airborne = 1;
            c->jump_sub = 0;
            c->jump_ctr = 0;
        }
        /* else grounded: world_y holds at ground_y, vvel 0. */
    }

    if (c->airborne) {
        if (c->jump_sub == 0) {
            /* WINDUP (case 3 sub 0, :834-841): count up each tick; on the tick the
             * counter exceeds CHAR_JUMP_WINDUP_THRESH (the 5th), apply the impulse +
             * one FALL grav step (the grav was selected from the pre-impulse vvel==0
             * = fall branch -> first sampled -76000) and advance to sub-state 1.  The
             * body is STATIONARY (no worldY change) through the 4 windup ticks. */
            c->jump_ctr = (int16_t)(c->jump_ctr + 1);
            if (c->jump_ctr > CHAR_JUMP_WINDUP_THRESH) {
                c->jump_sub = 1;
                c->jump_ctr = 0;
                c->vvel = CHAR_JUMP_IMPULSE;
                c->world_y += c->vvel / 100;
                c->vvel += CHAR_JUMP_FALL_GRAV;
            }
        } else {
            int32_t step = c->vvel / 100;
            if (c->vvel > 0 && c->world_y + step > c->ground_y) {
                /* a downward step that would penetrate the ground -> the collision
                 * mover returns blocked: clamp to the surface and zero vvel (line
                 * 926-931 -> 0x426f50 grounded). */
                c->world_y = c->ground_y;
                c->vvel = 0;
                c->airborne = 0;
                c->jump_sub = 0;
            } else if (c->vvel < 0) {
                /* RISING: variable-height decel — jump HELD floats up slowly (2000),
                 * RELEASED cuts the rise short (8000); release mid-rise to shorten it. */
                c->world_y += step;
                c->vvel += jump_held ? CHAR_JUMP_RISE_GRAV_HELD : CHAR_JUMP_RISE_GRAV_FREE;
            } else {
                /* FALLING: the (button-independent) fall accel, capped at terminal. */
                c->world_y += step;
                c->vvel += CHAR_JUMP_FALL_GRAV;
                if (c->vvel > CHAR_JUMP_FALL_TERMINAL) c->vvel = CHAR_JUMP_FALL_TERMINAL;
            }
        }
    }

    return dwx;
}

/* The dash-trigger half of 0x478ba0 (the run_mode != 2 default branch, the only
 * one runs/dash-window2 exercises).  Retail snapshots the prior command block
 * into local_608, zeroes 0x14854.., then per direction re-derives the command:
 *
 *   LEFT  (478ba0:152-198): iVar9 = FUN_00479e70(now,0,W,W, 2,2, 0)   // dtap id 2
 *     if (left_held(+0x11c)) {                                        // (run_mode!=2)
 *         if (local_608[0]==5 || iVar9) 0x14854 = 5;   // dash-left: sustain OR new tap
 *         else if (local_608[0]==1 || bVar1) 0x14854 = 1; }           // walk-left
 *   RIGHT (478ba0:200-246): the mirror with id 4, cmd 6 (dash) / 2 (walk).
 *
 * RIGHT runs second and writes the same slot, so it wins if both are held.  The
 * dash self-sustains via local_608[0]==5/6 (the prior frame's command) while the
 * direction stays held; releasing drops left_held -> the slot keeps its reset 0.
 * The walk latch (cmd 1/2, gated on bVar1 = the press-window auto-repeat) is
 * character_step's domain (its axis_held walk + warmup), so cmd_lr tracks only
 * the dash {0,5,6}; the prev==5/6 self-sustain reads identically either way. */
int character_resolve_run(character *c, const struct input_mgr *m, uint32_t now,
                          const int *axis_held, uint32_t window)
{
    if (c == NULL) return 0;

    int16_t prev = c->cmd_lr;          /* local_608[0] (the snapshot before reset) */
    int16_t cmd  = 0;                  /* 0x14854 reset to 0 each tick             */

    int left_held  = (axis_held != NULL) && axis_held[CHAR_AXIS_LEFT]  != 0;
    int right_held = (axis_held != NULL) && axis_held[CHAR_AXIS_RIGHT] != 0;

    /* Each block WRITES the slot when its direction is held (retail always does:
     * dash 5/6 on sustain-or-tap, else the walk value — here folded to 0 since
     * cmd_lr tracks only the dash and the walk is character_step's axis_held).
     * RIGHT runs second, so it overrides a held LEFT dash when both are held. */
    if (left_held) {                   /* +0x11c held (the run_mode!=2 gate)       */
        int dtap = (m != NULL) &&
                   input_dash_double_tap(m, now, INPUT_RING_DIR_LEFT, window);
        cmd = (prev == 5 || dtap) ? 5 : 0;         /* dash-left (sustain or new tap)*/
    }
    if (right_held) {                  /* +0x120 held; runs second, wins if both   */
        int dtap = (m != NULL) &&
                   input_dash_double_tap(m, now, INPUT_RING_DIR_RIGHT, window);
        cmd = (prev == 6 || dtap) ? 6 : 0;         /* dash-right                    */
    }

    c->cmd_lr = cmd;
    return (cmd == 5 || cmd == 6);     /* the run flag for character_step          */
}

/* The U/D-pose half of 0x478ba0 (:248-259), sibling to character_resolve_run.
 * Retail (after the L/R blocks) re-derives cmd[3] @ +0x14860 from the held axis,
 * the prior-tick snapshot (local_608[3]), and a single windowed ring find:
 *
 *   DOWN (478ba0:248-253): iVar10 = FUN_00479960(now,10,800,1, 3, ...)   // ring id 3
 *     if (down_held(+0x118) && (local_608[3]==10 || -1<iVar10 || held>240ms))
 *         0x14860 = 10;                              // crouch / slide intent
 *   UP   (478ba0:254-259): iVar10 = FUN_00479960(now,10,800,1, 1, ...)   // ring id 1
 *     if (up_held(+0x114)   && (local_608[3]==0xb || -1<iVar10 || held>240ms))
 *         0x14860 = 0xb;                             // defensive / door-enter intent
 *
 * UP runs second and writes the same slot, so it overrides a held DOWN.  The
 * 240 ms held-time arm (array_B +0x140/+0x144) is PORT-DEBT(char-pose-holdtime)
 * — for a continuous hold the ring (engages once the press ages past the 10 ms
 * floor) + the prev-sustain already keep the pose latched, so it is a redundant
 * backstop here; the door-enter probe (:260-285) is PORT-DEBT(char-up-door-probe). */
int16_t character_resolve_pose(character *c, const struct input_mgr *m,
                               uint32_t now, const int *axis_held)
{
    if (c == NULL) return 0;

    int16_t prev = c->cmd_pose;        /* local_608[3] (snapshot before reset)     */
    int16_t cmd  = 0;                  /* 0x14860 reset to 0 each tick             */

    int down_held = (axis_held != NULL) && axis_held[CHAR_AXIS_DOWN] != 0;
    int up_held   = (axis_held != NULL) && axis_held[CHAR_AXIS_UP]   != 0;

    if (down_held) {                   /* +0x118 held                              */
        int ring = (m != NULL) &&
            input_ring_find_recent(m, now, INPUT_RING_DIR_DOWN,
                                   CHAR_POSE_WINDOW_LO, CHAR_POSE_WINDOW_HI) >= 0;
        if (prev == CHAR_POSE_DOWN || ring) cmd = CHAR_POSE_DOWN;
    }
    if (up_held) {                     /* +0x114 held; runs second, wins if both   */
        int ring = (m != NULL) &&
            input_ring_find_recent(m, now, INPUT_RING_DIR_UP,
                                   CHAR_POSE_WINDOW_LO, CHAR_POSE_WINDOW_HI) >= 0;
        if (prev == CHAR_POSE_UP || ring) cmd = CHAR_POSE_UP;
    }

    c->cmd_pose = cmd;
    return cmd;
}

/* The SWORD unsheathe/sheathe TOGGLE — the freeroam Z key (ring 9).  Consume a
 * fresh ring-9 press (input_poll_consume: newest-first scan, .flag==1, age<=100ms,
 * consume-on-read) so each physical press flips the stance exactly once (the
 * consume zeroes the matched slot, so the next tick can't re-find the same press).
 * The draw/sheathe ANIM is arche_sword_clip's job (it watches sword_out's edge).
 *
 * PORT-DEBT(sword-quest-gate): retail only lets id 9 reach the draw once the
 * errands quest set weapon+0xd4=2 (case 8); the quest is unported, so the port
 * toggles freely in the errands freeroam (the caller already locks input during
 * the opening dialogue, so Z is dead until control hands off — matching retail's
 * post-tutorial enable point). */
int16_t character_resolve_sword(character *c, struct input_mgr *m, uint32_t now)
{
    if (c == NULL) return 0;
    if (m != NULL && input_poll_consume(m, now, INPUT_RING_SWORD))
        c->sword_out = (int16_t)(c->sword_out ? 0 : 1);
    return c->sword_out;
}

/* The sword-OUT ATTACK swing — see character.h.  The cmd[4] half of 0x478ba0:
 *   :296-299  X held (the +0x128 auto-attack level) + 200 ms since +0x154 -> cmd4=0xe
 *   :459-460  +0x66 (sword out) || cmd4==0xe -> cmd4=0xf  (the swing)
 * with the swing-complete lock (+0x68) that 442a70 enforces (a new swing can't start
 * until the current one finishes).  We read X off the held axis (the producer's
 * +0x128 level, input_live.c slot 5) rather than a ring, matching 478ba0:296.  The
 * swing's MOVEMENT is character_step (it brakes vel to 0 while `attacking`); the ANIM
 * is the clip layer.  `m` is unused for neutral (the held axis is the trigger) but
 * kept in the signature for the directional/ring paths chip 2b adds. */
/* The swing duration per kind — one source of truth shared by the movement lock
 * (here) and the clip (actor_spawn.c encodes the same total via its frame_delta).
 * RE'd off sword2.osr res 0x571 (see character.h). */
int character_attack_ticks(int attack_kind)
{
    switch (attack_kind) {
    case CHAR_ATTACK_FORWARD: return CHAR_ATTACK_FORWARD_TICKS;
    case CHAR_ATTACK_DOWN:    return CHAR_ATTACK_DOWN_TICKS;
    case CHAR_ATTACK_BACK:    return CHAR_ATTACK_BACK_TICKS;
    case CHAR_ATTACK_UP:      return CHAR_ATTACK_UP_TICKS;
    case CHAR_ATTACK_NEUTRAL:
    default:                  return CHAR_ATTACK_NEUTRAL_TICKS;
    }
}

int16_t character_resolve_attack(character *c, const struct input_mgr *m,
                                 uint32_t now, const int *axis_held)
{
    (void)m;
    if (c == NULL) return 0;

    /* A swing in progress advances to completion first (the +0x68 mid-swing lock):
     * one tick per call, clearing `attacking` when the KIND's duration elapses, so
     * the next call is free to re-trigger (a held/spammed X re-swings each completion
     * — sword2.osr plays 104-109 back-to-back through the attack-spam).  On a BACK
     * swing's completion, FLIP facing 1<->3 + negate the horizontal accumulator: the
     * 45e830:363-365 turn-around (the +0x54==4 branch flips +0x2c at sub-state 4 and
     * sets +0x28 = -+0x28).  vel is already braked to 0 by the lock, so the negate is
     * a no-op from rest; the facing flip leaves her facing the swung-toward direction,
     * matching sword2.osr's post-back idle jumping to the opposite +192 bank. */
    if (c->attacking) {
        c->attack_timer = (int16_t)(c->attack_timer + 1);
        if (c->attack_timer >= character_attack_ticks(c->attack_kind)) {
            c->attacking = 0;
            if (c->attack_kind == CHAR_ATTACK_BACK) {
                c->facing = (int16_t)((c->facing == CHAR_FACE_LEFT)
                                          ? CHAR_FACE_RIGHT : CHAR_FACE_LEFT);
                c->vel = -c->vel;
            }
        }
        return c->attacking;
    }

    /* Trigger: X held (the +0x128 attack level) + sword drawn + grounded + the 200 ms
     * auto-attack refractory since the last swing.  Pick the swing VARIANT from the
     * held direction vs facing (478ba0 builds cmd[4]=0xf; the sword-out form runs the
     * matching registered template, 41f200:1181-1201): UP beats DOWN beats a held L/R
     * (FORWARD if toward facing, BACK if away) beats no-direction NEUTRAL.  UP+X is the
     * 0x283f overhead thrust (registered in the same 0xc35b sword-out form, :1193/1202),
     * an OVERHEAD swing on bank 0x8d (res 0x572) — stationary like NEUTRAL.  UP and DOWN
     * are mutually exclusive on the dpad, so UP-first is unambiguous. */
    int x_held = (axis_held != NULL) && (axis_held[CHAR_AXIS_ATTACK] != 0);
    if (x_held && c->sword_out && !c->airborne &&
        (uint32_t)(now - c->attack_last_ms) >= CHAR_ATTACK_REFRACTORY_MS) {
        int kind = CHAR_ATTACK_NEUTRAL;
        if (axis_held != NULL) {
            int up = axis_held[CHAR_AXIS_UP]    != 0;
            int dn = axis_held[CHAR_AXIS_DOWN]  != 0;
            int l  = axis_held[CHAR_AXIS_LEFT]  != 0;
            int r  = axis_held[CHAR_AXIS_RIGHT] != 0;
            if (up) {
                kind = CHAR_ATTACK_UP;
            } else if (dn) {
                kind = CHAR_ATTACK_DOWN;
            } else if (l != r) {                  /* exactly one of L/R held          */
                int dir  = r ? +1 : -1;           /* +1 right / -1 left               */
                int face = (c->facing == CHAR_FACE_LEFT) ? -1 : +1;
                kind = (dir == face) ? CHAR_ATTACK_FORWARD : CHAR_ATTACK_BACK;
            }
        }
        c->attacking      = 1;
        c->attack_timer   = 0;
        c->attack_kind    = (int16_t)kind;
        c->attack_last_ms = now;
    }
    return c->attacking;
}
