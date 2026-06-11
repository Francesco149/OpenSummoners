/*
 * character.{c,h} — the controllable CHARACTER movement reduction (Phase-4 chip 3,
 * the milestone: Arche walks).  Ports the freeroam mover that retail runs as TWO
 * layers, BOTH shared with the existing actor system (engine-quirk #101):
 *
 *   AI / intent   0x478ba0  the RNG-free character update (townsfolk + Arche
 *                           share it; counterpart of the butterfly's 0x47b990).
 *                           Reads the HELD-AXIS array (input-mgr +0x114 U / +0x118
 *                           D / +0x11c L / +0x120 R, quirk #41) and builds the
 *                           command block entity+0x14854: LEFT->[0]=1 (walk)/5
 *                           (run), RIGHT->2/6, DOWN->[3]=10, UP->[3]=0xb.
 *   APPLY / commit 0x442a70 the shared kinematic integrator (the SAME apply pass
 *                  case 0x75 that integrates the butterflies, 0x485fc0+0x96e ->
 *                  0x442a70(cmd, body, body, 0, 0)).  case 0x75 reads cmd[0] and
 *                  ramps the horizontal velocity accumulator body+0x28 via the
 *                  150-byte clamp-ramp 0x445db0, then commits worldX += vel/100
 *                  through the collision-aware mover 0x54db10 (flat town street =
 *                  no clamp).  facing body+0x2c flips 1<->3 at v==0.
 *
 * ── The WALK law (ground-truthed bit-exact, runs/mover-caller; ckpt 114) ──
 * Arche's real body (0xe637b80) tracked tick-for-tick over a held-axis walk:
 *   accelerate (held): vel += 1600/tick toward the cap 24000  -> dwx = vel/100
 *                      ramps +16/tick (16,32,48,..,240) to the +240 cap.
 *   brake (released):  vel -= 800/tick toward 0  -> dwx ramps -8/tick (240,..,8,0).
 *   facing:            holds through the brake-to-stop, flips 1<->3 only at v==0
 *                      when the opposite direction is commanded (then accelerates).
 * The horizontal accumulator is body+0x28 (NOT body+0x18, which is the VERTICAL /
 * jump velocity and reads 0 the whole flat walk — reconciling the ckpt-113 note).
 *
 * ── REDUCTION scope (PORT-DEBT, mirrors the butterfly chip-1 reduction) ──
 * The real 0x442a70 is a 12 KB shared integrator (the per-actor animation state
 * machine body+0x38/+0x3a/+0x3c, run/jump/skills, entity + tile collision sweeps,
 * slopes).  Here we port only the OPEN-AIR FLAT WALK that the capture exercises:
 *   - the velocity ramp (accel 1600 / cap 24000 / brake 800), capture-exact;
 *   - the worldX commit as a flat reduction of 0x54db10 (no collision clamp —
 *     the town street is flat; the chip-2 collision_move_vertical sibling + the
 *     horizontal mover wire in when a grounded actor needs them).
 * DEFERRED, tagged in docs/port-debt.md:
 *   PORT-DEBT(char-run-jump)        run (cmd 5/6, higher cap) + jump (cmd[2]/[4])
 *                                   — need the run/jump-scancode capture first.
 *   PORT-DEBT(char-input-autorepeat) the press->latch warmup is retail's wall-clock
 *                                   auto-repeat threshold in 0x478ba0; the constant
 *                                   CHAR_INPUT_REPEAT_DELAY reproduces the seed-
 *                                   pinned capture (motion starts 2 idle ticks
 *                                   after the press), not the real ms threshold.
 *   PORT-DEBT(char-walk-tuning)     accel/cap/brake are the captured constants;
 *                                   the real source is the per-entity move-tuning
 *                                   in_ECX[0x565b/0x565c/0x565e] (un-captured VAs).
 *   PORT-DEBT(char-collision-mover) the flat worldX commit; the collision-aware
 *                                   0x54db10 (+ the reverse-decel rate, untested —
 *                                   the capture has an idle gap, no direct reverse).
 *
 * Win32-free + pure (advances only its own body state); host-tested field-exact
 * vs the capture (tests/test_character.c).  See docs/plans/movement-system.md
 * (chip 3) + engine-quirk #101.
 */
#ifndef OSS_CHARACTER_H
#define OSS_CHARACTER_H

#include <stddef.h>
#include <stdint.h>

/* The held-axis slots the character AI reads (held_trace.h / quirk #41 order). */
enum {
    CHAR_AXIS_UP    = 0,   /* input-mgr +0x114 */
    CHAR_AXIS_DOWN  = 1,   /* input-mgr +0x118 */
    CHAR_AXIS_LEFT  = 2,   /* input-mgr +0x11c */
    CHAR_AXIS_RIGHT = 3,   /* input-mgr +0x120 */
    CHAR_AXIS_COUNT = 4,
};

/* The walk velocity law (capture-exact constants; PORT-DEBT(char-walk-tuning) for
 * the real per-entity move-tuning source in_ECX[0x565b/c/e]).  vel is the signed
 * horizontal accumulator body+0x28; worldX advances by vel/100 each tick. */
#define CHAR_WALK_ACCEL  1600    /* vel += 1600/tick held  -> dwx +16/tick        */
#define CHAR_WALK_CAP    24000   /* |vel| cap              -> dwx cap +-240        */
#define CHAR_WALK_BRAKE   800    /* vel -= 800/tick on release -> dwx -8/tick      */

/* ── The RUN (dash) velocity law (0x442a70 case 0x75, the cmd[0]=5/6 branch; chip 3b,
 * ckpt 118) ──  A direction DOUBLE-TAP (the event ring, detected by 0x479e70 in the AI
 * 0x478ba0) latches the run; it SELF-SUSTAINS while the direction is held and ends on
 * release.  In the apply integrator the run differs from the walk in exactly TWO captured
 * per-entity consts + nothing else (same body+0x28 accumulator, same 0x445db0 ramp, same
 * -800 brake) — VALIDATED BIT-EXACT vs retail's per-tick body (runs/runjump-gt/capdash2,
 * ckpt 118: a ring-injected double-tap RIGHT from the town freeroam):
 *   - RUN cap   in_ECX[0x5664] = 48000  -> dwx cap +-480 (exactly 2x the walk's +-240).
 *   - RUN accel in_ECX[0x565d] =  3200  -> +-32/tick, but ONLY while |vel| < the WALK cap
 *     24000 (decompile 442a70:998 `hvel < param_3`, param_3 still the walk cap there);
 *     from 24000 up to 48000 the accel DROPS to the walk accel 1600.  So the run ramp is
 *     TWO-PHASE: +3200/tick to 24000 (the captured 1600,3200 walk ticks then 6400..25600),
 *     then +1600/tick to 48000 (27200,28800,..,48000).
 * Releasing the dash while still holding the direction (cmd[0]->2 walk, |vel| over the now-
 * walk cap) decelerates 48000->24000 at the BRAKE rate (the 0x445db0 over-cap path reduces
 * by local_18 = -800), then walks at 24000.  The double-tap DETECTION that produces the run
 * flag is the AI's job (0x479e70 ring scan), deferred to the live wire -> the `run` arg of
 * character_step is the resolved cmd[0]==5/6 (PORT-DEBT(char-run-trigger)). */
#define CHAR_RUN_CAP    48000    /* in_ECX[0x5664]  |vel| cap when running -> dwx +-480 */
#define CHAR_RUN_ACCEL   3200    /* in_ECX[0x565d]  accel while |vel| < CHAR_WALK_CAP    */

/* ── The JUMP law (0x442a70 case 3 = the body+0x38==3 airborne sub-FSM, chip 3b) ──
 * vvel is the signed VERTICAL accumulator body+0x18; worldY advances by vvel/100
 * each airborne tick (worldY grows DOWNWARD — ground is the larger value, apex the
 * smaller).  Gravity is ASYMMETRIC and VARIABLE-HEIGHT (the floaty rise ~ Arche's
 * reputation); the impulse + the two rise gravs are per-entity move-tuning consts
 * CAPTURED LIVE off Arche's entity (ckpt 117, runs/runjump-gt/capconsts — in_ECX[idx]
 * = entity byte idx*4), NOT decompile-guessed:
 *   - IMPULSE   in_ECX[0x5667] = -80000  set when the windup anim completes (case 3
 *                               sub-state 0, counter>4; line 837) -> vvel.
 *   - RISE grav while RISING (vvel<0) selects on the jump button (line 858-866):
 *       HELD (cmd[2]!=0): in_ECX[0x5668] = 2000  -> a slow decel = the FLOATY HIGH
 *                         jump (apex ~38 ticks; the variable-height hold).
 *       FREE (cmd[2]==0): in_ECX[0x5669] = 8000  -> a fast decel = the SHORT HOP
 *                         (apex 10 ticks).  The captured arc is the short hop: the
 *                         ring-injected execute (cmd[2]=7) is a ONE-tick event, so
 *                         cmd[2]==0 the whole rise (verified, runs/runjump-gt).
 *   - FALL grav while FALLING (vvel>=0) AND on the launch tick = 4000.  Pinned
 *                         bit-exact by the arc; its source is NOT in the move-tuning
 *                         band 0x565a..0x566f (captured) — a GLOBAL/derived gravity
 *                         (note 4000 = 8000/2 = the free rise grav halved) ->
 *                         PORT-DEBT(char-jump-fall-grav-source).
 * The launch tick's first SAMPLED vvel is -76000, not the -80000 impulse, because
 * the launch applies one FALL grav step (the grav was selected from the pre-impulse
 * vvel==0 = the fall branch): -80000 + 4000 = -76000.  The captured 27-tick arc
 * (the SHORT HOP) is reproduced bit-exact (tests/test_character.c); the HELD high
 * jump uses the captured 2000 const + the RE'd branch but is not yet bit-exact
 * validated against a held-button capture -> PORT-DEBT(char-jump-variable-height). */
#define CHAR_JUMP_IMPULSE        (-80000)  /* in_ECX[0x5667]  vvel at launch        */
#define CHAR_JUMP_RISE_GRAV_HELD    2000   /* in_ECX[0x5668]  rising, jump HELD      */
#define CHAR_JUMP_RISE_GRAV_FREE    8000   /* in_ECX[0x5669]  rising, jump RELEASED  */
#define CHAR_JUMP_FALL_GRAV         4000   /* falling + the launch tick (global/der.)*/
/* Terminal fall velocity — vvel caps here on the way down (observed bit-exact: the
 * held high jump plateaus at 64000 for 8 ticks while still falling, ckpt 117
 * runs/runjump-gt/capheld).  Source NOT in the move-tuning band (= the fall grav's
 * cousin, 64000 = 16*4000) -> PORT-DEBT(char-jump-fall-grav-source).  Safe for the
 * short hop (it reaches exactly 64000 at the landing tick, so the clamp is a no-op). */
#define CHAR_JUMP_FALL_TERMINAL    64000

/* The press->latch warmup (PORT-DEBT(char-input-autorepeat)).  A fresh direction
 * press must persist this many ticks before the walk command latches; the capture
 * shows motion start 2 idle ticks after the press, so the third held tick latches
 * and accelerates.  Once latched it self-sustains while held (the local_608[0]
 * "was-walking" carry in 0x478ba0). */
#define CHAR_INPUT_REPEAT_DELAY  3

/* body+0x2c facing: 1 = right (+X), 3 = left (-X). */
#define CHAR_FACE_RIGHT  1
#define CHAR_FACE_LEFT   3

typedef struct {
    int32_t world_x;   /* body+4    — world X (the rendered position)            */
    int32_t world_y;   /* body+8    — world Y (grows DOWNWARD; jump arc)         */
    int32_t vel;       /* body+0x28 — signed horizontal velocity accumulator     */
    int32_t vvel;      /* body+0x18 — signed VERTICAL velocity accumulator (jump)*/
    int32_t ground_y;  /* the resting worldY (flat-town clamp; the real terrain  */
                       /*   surface comes from the collision mover, chip 4)      */
    int16_t facing;    /* body+0x2c — 1 right / 3 left                           */
    int16_t cmd_dir;   /* the latched walk direction (+1 right / -1 left / 0 idle)*/
    int16_t warm;      /* held-tick counter for the press->latch warmup          */
    int16_t held_dir;  /* last tick's commanded direction (re-arms warm on change)*/
    int16_t airborne;  /* body+0x38==3 — in the airborne (jump) state            */
    int16_t jump_held; /* last tick's jump button (for the launch rising edge)   */
} character;

/* Initialise a character at a spawn world position + facing (1 right / 3 left), at
 * rest on the ground (world_y == ground_y, vvel 0). */
void character_init(character *c, int32_t spawn_world_x, int32_t spawn_world_y,
                    int facing);

/* One sim-tick of the controllable character: `axis_held[0..3]` are the UP/DOWN/
 * LEFT/RIGHT held booleans (held_trace replay / the real producer 0x46a880);
 * `jump_held` is the jump button level (C; the AI's cmd[2], producer slot +0x124);
 * `run` is the resolved RUN flag (the AI's cmd[0]==5/6 — a direction double-tap held;
 * the 0x479e70 ring detection is deferred to the live wire, PORT-DEBT(char-run-trigger)).
 * Runs the AI reduction (held axis -> latched walk direction) then the apply reduction:
 * the horizontal velocity ramp (walk OR run cap/accel) + worldX commit + facing flip
 * (0x442a70 case 0x75) AND the vertical jump integrator (case 3 airborne).  Returns the
 * worldX delta applied this tick (dwx = vel/100); the jump arc is read from world_y/vvel.
 * The jump triggers on the rising edge of `jump_held` while grounded. */
int32_t character_step(character *c, const int *axis_held, int jump_held, int run);

#endif /* OSS_CHARACTER_H */
