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
 *   PORT-DEBT(char-reverse-decel)   the reverse-decel rate (commanding the
 *                                   opposite direction while moving): decompile
 *                                   says -accel, port uses the brake — untested
 *                                   (the captures idle before reversing).
 *   char-turn-state  PORTED (ckpt 177): the STANDING TURN-AROUND.  A from-rest
 *                                   REVERSAL no longer snaps — it plays retail's
 *                                   8-tick pivot: HOLD stationary facing-held for
 *                                   CHAR_TURN_HOLD ticks (the fr-6 windup), then
 *                                   flip facing + begin the walk ramp (the fr-7 ->
 *                                   +152 fr-159 cel lingers CHAR_TURN_HOLD more
 *                                   render ticks).  GROUND TRUTH = retail-stairs
 *                                   res 0x570 draw stream: fr 6 x4 (2951-4, hvel 0)
 *                                   -> 159 x4 (2955-8) -> walk 160 (2959); the
 *                                   walk onset delayed exactly 4 ticks = the -960
 *                                   wx the old instant flip caused.  turn_ctr drives
 *                                   the sim hold + flip, turn_frame the render cel
 *                                   (character_turn_frame -> ARCHE_TURN_CLIP).
 *
 * Win32-free + pure (advances only its own body state); host-tested field-exact
 * vs the capture (tests/test_character.c).  See docs/plans/movement-system.md
 * (chip 3) + engine-quirk #101.
 */
#ifndef OSS_CHARACTER_H
#define OSS_CHARACTER_H

#include <stddef.h>
#include <stdint.h>

#include "collision.h"   /* the tile movers + coll_slope_fn (the live wiring) */

/* The held-axis slots the character AI reads (held_trace.h / quirk #41 order). */
enum {
    CHAR_AXIS_UP    = 0,   /* input-mgr +0x114 */
    CHAR_AXIS_DOWN  = 1,   /* input-mgr +0x118 */
    CHAR_AXIS_LEFT  = 2,   /* input-mgr +0x11c */
    CHAR_AXIS_RIGHT = 3,   /* input-mgr +0x120 */
    CHAR_AXIS_COUNT = 4,
    CHAR_AXIS_JUMP   = CHAR_AXIS_COUNT,      /* axis_held[4] = jump  (C, +0x124) */
    CHAR_AXIS_ATTACK = CHAR_AXIS_COUNT + 1,  /* axis_held[5] = attack(X, +0x128) */
};

/* The SWORD-OUT ATTACK (cmd[4]=0xf, 478ba0:296-303 + the swing body state 442a70).
 * X held (axis_held[5] = the +0x128 attack level) + sword_out + grounded + not
 * already mid-swing + >= 200 ms since the last swing -> start a swing.  Which swing
 * is picked from the held direction at trigger time (CHAR_ATTACK_*); chip 2a ports
 * the NEUTRAL swing (no direction).  Ground-truthed off sword2.osr res 0x571:
 *   NEUTRAL  cels 104->109, dur-6 each = 36 ticks, STATIONARY (idle resumes at the
 *            same x).  The 200 ms refractory + the swing duration (36t > 200 ms)
 *            mean a held/spammed X re-swings right after each completes (the input
 *            recording's attack-spam plays 104-109 back-to-back). */
#define CHAR_ATTACK_REFRACTORY_MS  200u  /* 478ba0:297 auto-attack gate (X-held)   */
/* The swing VARIANT, picked at trigger time from the held direction vs facing
 * (chip 2b).  Retail registers each as a distinct action+variant template in the
 * sword-out form install 0x41f200:1181-1201 (0x27d9 var0/4, 0x27da var0/1/8) with
 * per-variant movement; the per-frame handler is 0x45e830 (the 0xc35b-only
 * delegate, 442a70:357-369).  We pick the kind from the held axis + facing and
 * play the captured cels (sword2.osr res 0x571):
 *   NEUTRAL  no direction       -> 104-109  STATIONARY        (0x27d9 var0, D=0)
 *   FORWARD  held dir == facing  -> 120-126  LUNGE +54px fwd   (forward step toward
 *                                            facing via 0054db10, 0x447ed0)
 *   DOWN     DOWN held           -> 112-115  STATIONARY, ->crouch
 *   BACK     held dir == ~facing  -> 144-148  TURNS AROUND      (0x27d9 var4, the
 *                                            +0x54==4 branch flips facing +0x2c and
 *                                            negates vel +0x28, 45e830:363-365)
 *   UP       UP held             -> an overhead THRUST on a SEPARATE BANK 0x8d (res
 *                                            0x572) cels 0-5, STATIONARY, + the sword-
 *                                            tip TRAIL vfx (res 0x40b) — chip 2c. */
enum {
    CHAR_ATTACK_NEUTRAL = 0,   /* X, no direction -> cels 104-109                    */
    CHAR_ATTACK_FORWARD,       /* X + dir toward facing -> 120-126 + forward lunge   */
    CHAR_ATTACK_DOWN,          /* X + DOWN -> 112-115 (crouch-attack)                */
    CHAR_ATTACK_BACK,          /* X + dir away from facing -> 144-148 + turn-around  */
    CHAR_ATTACK_UP,            /* X + UP -> bank 0x8d (res 0x572) cels 0-5 overhead  */
};
/* Per-kind swing duration in sim-ticks (= the matching clip's total cels*durs, kept
 * 1:1 so the movement lock and the anim end together).  RE'd off sword2.osr res 0x571
 * tick-aligned to sword2-input.jsonl (durs in actor_spawn.c's clip frame_delta):
 *   NEUTRAL 6 cels * dur-6 = 36   (3485-3520)
 *   FORWARD 7 cels * dur-6 = 42   (3792-3833)
 *   DOWN    durs [8,6,5,7] = 26   (3957-3982)
 *   BACK    durs [4,4,7,7,5] = 27 (4082-4108)
 *   UP      durs [4,4,4,8,8,8] = 36 (res 0x572 cels 0-5, ticks 3880-3915) */
#define CHAR_ATTACK_NEUTRAL_TICKS  36
#define CHAR_ATTACK_FORWARD_TICKS  42
#define CHAR_ATTACK_DOWN_TICKS     26
#define CHAR_ATTACK_BACK_TICKS     27
#define CHAR_ATTACK_UP_TICKS       36

/* The SWORD-DRAW STARTUP latency (ckpt 163f) — retail's Z press does NOT engage the
 * sword-out form on the press tick; it QUEUES a context-action (478ba0:477 ->
 * 0x479de0 writes the cmd block entity+0x14868 = the action, +0x14870 = type 0xd2)
 * that the per-tick integrator's action FSM executes after a startup: 442a70 dispatches
 * per current form (case 0xc35a sword-in -> 0x45a300(.., &DAT_0062f868, 0xd2)), and
 * 45a300 (a 14 KB action-execution state machine, UNPORTED) runs the draw action's
 * startup phase before re-installing Arche to the sword-out form 0xc35b (= bank 0x8c,
 * res 0x571).  So the form swaps — and res 0x571 fr96 first appears — ~3-4 ticks AFTER
 * the press; through the startup Arche stays in the sword-IN idle (res 0x570 fr0).
 * GROUND TRUTH (sword2.osr, sword_cels.py, FRAMEBEG axis): res 0x570 idle LAST at tick
 * 1809, res 0x571 fr96 FIRST at 1810 — vs the port (no delay) swapping at 1806, i.e. 4
 * FRAMEBEG-ticks early.  We model the startup as a pending timer that defers the
 * sword_out engagement (the bank swap + the draw/sheathe clip's edge both key off
 * sword_out, so both shift together).  PORT-DEBT(sword-draw-startup): the value is
 * calibrated to land the draw at the recording's FRAMEBEG onset (1810) pending a port
 * of the 45a300 action FSM that would make the latency EMERGE; the same queued-command
 * pipeline drives the sheathe (1->0), so the model is symmetric. */
#define CHAR_SWORD_DRAW_STARTUP    4
/* The FORWARD lunge: total world-X displacement toward facing over the swing.  RE'd
 * mechanism = a direct world_x step toward facing through the collision mover 0054db10
 * (0x447ed0, sign-flipped on facing==3); the swing locks vel like neutral and the
 * step rides on top.  The MAGNITUDE is captured ground truth (sword2.osr: the post-swing
 * idle cel lands +54px = +5400 world from the pre-swing idle, the camera then easing to
 * follow) — PORT-DEBT(sword-attack-gameplay): the exact per-substate displacement values
 * live in the 0x27da forward template (0x4287d0), un-traced through the unreliable
 * 45e830 combo path; this even-distributes the captured net. */
#define CHAR_ATTACK_FORWARD_LUNGE  5400

/* The swing duration for a kind (one source of truth for the timer + the clip). */
int character_attack_ticks(int attack_kind);

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

/* ── The STANDING TURN-AROUND (char-turn-state, ckpt 177) ─────────────────────
 * On a from-rest reversal retail plays an 8-tick pivot (the 0x442a70 STATE-1 FSM
 * -> the FUN_0040a540 turn action; the cels are res 0x570 fr 6/7 on bank 0x8b):
 *   ticks 1..CHAR_TURN_HOLD  = the WINDUP: stationary (vel held at 0), facing
 *                              UNCHANGED, render cel fr 6 (turn_frame 0).
 *   tick  CHAR_TURN_HOLD+1    = FLIP facing to the commanded dir + begin the walk
 *                              ramp (vel accelerates from 0 as normal).
 *   ticks CHAR_TURN_HOLD+1..CHAR_TURN_TOTAL = the walk ramp runs, but the render
 *                              still shows the flipped turn cel fr 7 (-> +152 =
 *                              fr 159 when facing left; turn_frame 1) — a render
 *                              linger over the accelerating walk.
 *   tick  CHAR_TURN_TOTAL+1   = the pivot is complete: the normal walk clip (fr
 *                              160) takes over.
 * MEASURED off the retail-stairs draw stream (fr 6 held 4 ticks, fr 159 held 4)
 * — the clip's exact frame_dur (the un-ported 0x45e830 turn form) stands in as
 * these two constants, the same measured-clip pattern as ARCHE_WALK_CLIP
 * (char-walk-anim-distance).  The 4-tick windup reproduces the -960 wx reversal
 * offset bit-exact (see character.c). */
#define CHAR_TURN_HOLD    4      /* stationary windup ticks before the facing flip   */
#define CHAR_TURN_TOTAL   8      /* total pivot ticks (windup + the fr-159 linger)   */

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
 * jump's 2000 branch is bit-exact validated too (ckpt 117, runs/runjump-gt/capheld,
 * test_character_jump_held_rise).  The held apex's town-CEILING clamp now EMERGES
 * from the vertical collision mover against the room grid (ckpt 175). */
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

/* ── The jump WINDUP (0x442a70 case 3 sub-state 0, decompile :834-841; chip 3b,
 * ckpt 119) ──  The jump EXECUTE (cmd[2]==7 -> 0x426f50(body,3) sets body+0x38=3
 * main state, +0x3a=0 sub-state, +0x3c=0 counter) enters the airborne state IMMEDIATELY
 * but the body stays STATIONARY (vvel 0) while a counter increments once per tick; the
 * IMPULSE fires on the tick the counter exceeds 4 (the 5th tick in sub-0), advancing to
 * sub-state 1.  So there are 4 stationary windup ticks — a visible launch-anticipation
 * (crouch) ~8 flips long — between the trigger and the launch.  GROUND-TRUTHED bit-exact
 * from the bstate (body+0x38 = main | sub<<16) field of the ring-injected capture
 * (runs/runjump-gt/capjump-ring2): flips 4602-4609 read (main 3, sub 0, vvel 0) = the 4
 * windup ticks, flip 4610 reads (main 3, sub 1, vvel -76000) = the impulse.  Invisible
 * to the earlier arc extraction (jump_arc.py keys on vvel!=0) but visible live. */
#define CHAR_JUMP_WINDUP_THRESH  4   /* impulse fires when the counter exceeds this (5th tick) */

/* The press->latch warmup (PORT-DEBT(char-input-autorepeat)).  A fresh direction
 * press must persist this many ticks before the walk command latches.  Retail is
 * exactly 1 IDLE TICK: the clean tick-axis proxy re-drive reading the engine's own
 * input fields (runs retail-decomp.osr, OSS_OSR_STATE cmd0/lvlR/edgeR) shows, for a
 * right-press at tick T: T-> lvlR 0->1 + the rising-edge ts stamped (0x468a20), cmd0
 * still 0 (the 0x478ba0:182 `now-edge_ts < 0xb` 11 ms gate not yet crossed); T+1->
 * cmd0=2 + hvel 1600 + wx moves (now-edge = the ~16 ms lockstep step >= 11 ms).  So
 * motion latches on the 2nd held tick.  (The old DELAY=3 / "2 idle ticks" was tuned
 * to the FRIDA flip-axis GT `mover-caller`/`capdash2`, which carried a +1: a held
 * entry "flip N" only takes effect at frame N+1's input poll because the agent's
 * flip counter increments at the PRESENT, after the poll — the same +1 class fixed
 * port-side (feed_input) and proxy-side (EI_TICK_AXIS).  findings/freeroam-brake-onset.md.)
 * Once latched it self-sustains while held (the local_608[0] "was-walking" carry in
 * 0x478ba0). */
#define CHAR_INPUT_REPEAT_DELAY  2

/* ── The UP/DOWN POSE command (0x478ba0:248-259, the cmd[3] @ entity+0x14860) ──
 * The vertical-direction intent the char-AI resolves alongside the L/R walk/dash:
 *   DOWN (held +0x118) -> cmd[3] = 10   (0x478ba0:248-253) — crouch in place, or
 *                                        SLIDE if entered from a dash (apply state 6)
 *   UP   (held +0x114) -> cmd[3] = 0xb  (0x478ba0:254-259) — a DEFENSIVE pose that
 *                                        kills the walk/dash accel (apply states 2/5
 *                                        skip the accel ramp -> brake to a faster stop),
 *                                        and the door/ledge ENTER (the 0x442a70 probe
 *                                        block :260-285 — PORT-DEBT(char-up-door-probe)).
 * Each engages when its direction is HELD *and* one of: the prior tick already posed
 * this way (self-sustain, local_608[3]); a fresh direction ring press aged in the
 * [10,800] ms window (the primary trigger, FUN_00479960); or the press has been held
 * past the 240 ms input buffer (array_B +0x140/+0x144 — PORT-DEBT(char-pose-holdtime),
 * a redundant backstop the ring + sustain already cover for a continuous hold).
 * The APPLY-side physics (the velocity effect of states 2/5/6) is the next chip and
 * needs the live const capture (0x442a70's case-0x75 horizontal ramp has unreliable
 * decompile var-reuse — RE the structure, pin the values by capture; ckpt-117 lesson). */
#define CHAR_POSE_DOWN        10    /* cmd[3] = 10  (DOWN: crouch / slide)            */
#define CHAR_POSE_UP          0xb   /* cmd[3] = 0xb (UP: defensive stop / door-enter) */
#define CHAR_POSE_WINDOW_LO   10u   /* FUN_00479960 lo window (ms): the 1-frame buffer */
#define CHAR_POSE_WINDOW_HI   800u  /* FUN_00479960 hi window (ms)                     */

/* body+0x2c facing: 1 = right (+X), 3 = left (-X). */
#define CHAR_FACE_RIGHT  1
#define CHAR_FACE_LEFT   3

/* Arche's physics-body BOX (body+0xc/+0x10/+0x14) — live-probed off retail
 * during the errands freeroam (runs/arche-box, tools/flow/arche_box_fields.json;
 * refreshed each tick from the entity config in_ECX[10]/[0xb], 442a70:119-120 —
 * constant for Arche).  world_x/world_y are the box TOP-LEFT (body+4/+8). */
#define CHAR_BOX_W       2000
#define CHAR_BOX_H       5600
#define CHAR_BOX_MARGIN  0

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
    int16_t jump_sub;  /* body+0x3a — airborne sub-state: 0 = windup, 1 = launched*/
    int16_t jump_ctr;  /* body+0x3c — the windup tick counter (case 3 sub 0)     */
    int16_t cmd_lr;    /* entity+0x14854 L/R command, the DASH subset: 0 = not    */
                       /*   dashing, 5 = dash-left, 6 = dash-right (the self-     */
                       /*   sustain state for character_resolve_run; walk 1/2 is  */
                       /*   character_step's axis_held domain — see that fn)      */
    int16_t cmd_pose;  /* entity+0x14860 cmd[3] U/D pose: 0 none / 10 DOWN (crouch */
                       /*   /slide) / 0xb UP (defensive) — the self-sustain state  */
                       /*   for character_resolve_pose (local_608[3] snapshot)     */
    int16_t sword_out; /* the SWORD stance: 0 = sheathed (in) / 1 = drawn (out).   */
                       /*   Engaged by Z (ring 9) via character_resolve_sword AFTER */
                       /*   the CHAR_SWORD_DRAW_STARTUP latency (the queued context- */
                       /*   action pipeline); the anim FSM (arche_sword_clip) plays  */
                       /*   the draw/sheathe transient on the edge, and freeroam_step */
                       /*   swaps the body bank (0x8b<->0x8c) off it.  No quest gate  */
                       /*   (ckpt 159: Z draws on a fresh new game).                  */
    int16_t sword_pending;       /* 1 while a queued draw/sheathe command is in its   */
                                 /*   startup (sword_out not yet flipped) — the model  */
                                 /*   of retail's cmd[5]/0xd2 -> 45a300 action FSM.    */
    int16_t sword_pending_timer; /* sim-ticks remaining until the queued toggle engages*/
    int16_t sword_pending_target;/* the sword_out value to apply when the timer elapses*/
    int16_t attacking;     /* body+0x68 mid-swing: 0 = not / 1 = swinging.  Set by  */
                           /*   character_resolve_attack on the X trigger; while !=0 */
                           /*   the apply locks movement (neutral = stationary) and  */
                           /*   the anim plays the swing clip; cleared when the swing */
                           /*   completes (attack_timer reaches the kind's duration).*/
    int16_t attack_timer;  /* sim-ticks elapsed in the current swing (drives the     */
                           /*   duration; body+0x66 is retail's drain countdown).    */
    int16_t attack_kind;   /* which swing (CHAR_ATTACK_*): neutral now, directionals */
                           /*   (fwd/down/back/up) chip 2b.  Picked at trigger time   */
                           /*   from the held dir; the clip layer keys the cels off it.*/
    uint32_t attack_last_ms;/* body+0x154: the last swing's fire time — the 200 ms   */
                           /*   auto-attack refractory (478ba0:297, X-held repeat).  */

    /* ── the LIVE COLLISION wiring (the 442a70 tick geometry, ckpt 175) ──────
     * coll_grid = the current room's runtime grid (town_render.grid); the
     * movers then make terrain (stairs/walls/ceilings) EMERGE.  NULL = the
     * open-air flat reduction (host tests; never NULL in the live build).
     * coll_slope/_ctx resolve region-B slope refs (main.c: exe_data_bytes off
     * the user's exe .rdata — retail reads the same bytes off its own image). */
    const uint8_t *coll_grid;
    coll_slope_fn  coll_slope;
    void          *coll_slope_ctx;
    int16_t        supported;  /* body+0x24 — the tick-start SUPPORT probe result
                                  (442a70:104-118, a delta=+1 vertical probe;
                                  blocked == standing on ground).  Drives the
                                  ledge walk-off -> FALL transition (:937). */
    int16_t        turn_ctr;   /* the STANDING TURN-AROUND pivot tick index: 0 =
                                  not turning; 1..CHAR_TURN_TOTAL = mid-pivot (a
                                  from-rest reversal, char-turn-state).           */
    int16_t        turn_frame; /* the render cel for THIS tick: -1 = not turning
                                  (walk/idle clip), 0 = the fr-6 windup cel, 1 =
                                  the fr-7 (-> +152 fr-159) flipped cel.  main.c
                                  reads it via character_turn_frame().            */
} character;

/* The dash double-tap WINDOW (ms): the config field *(*0x8a6e80 + 0xf8) the
 * char-AI 0x478ba0 passes to FUN_00479e70 as both window params.  Read LIVE
 * from retail (no static default — it lives in the DInput god-object built at
 * engine init): runs/dash-window2 = 800, with run_mode *(*0x8a6e80+0x510) == 0
 * (!= 2 -> the normal double-tap branch is the active path, which this ports).
 * Two same-direction presses within 800 ms (each within 800 ms of "now") =
 * a dash. */
#define CHAR_DASH_WINDOW_MS  800u

/* The L/R direction ring ids the dash scans (input.h INPUT_RING_DIR_*). */

struct input_mgr;  /* fwd — the AI reads the event ring (input.h); keep this header light */

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
 * (0x442a70 case 0x75) AND the vertical jump integrator (case 3 airborne).  Also consumes
 * `c->cmd_pose` (set by character_resolve_pose): while a U/D pose is engaged + grounded the
 * apply states 2/5 disable the accel and brake the velocity toward 0 at the WALK brake
 * (crouch / slide / UP-stops-you-faster — bit-exact -800/tick, ckpt 153).  Returns the
 * worldX delta applied this tick (dwx = vel/100); the jump arc is read from world_y/vvel.
 * The jump triggers on the rising edge of `jump_held` while grounded: the body enters the
 * airborne state at once but stays stationary for a 4-tick WINDUP (CHAR_JUMP_WINDUP_THRESH)
 * before the launch impulse fires (case 3 sub-state 0). */
int32_t character_step(character *c, const int *axis_held, int jump_held, int run);

/* The STANDING TURN-AROUND render cel for the current tick (set by character_step):
 * -1 = not turning (use the walk/idle clip), 0 = the fr-6 windup cel, 1 = the fr-7
 * flipped cel (-> +152 fr-159 when facing left).  main.c's freeroam render forces
 * ARCHE_TURN_CLIP's frame to this while it is >= 0.  See CHAR_TURN_* / character.c. */
int character_turn_frame(const character *c);

/* Resolve the RUN (dash) flag from live input — the dash-trigger half of the
 * char-AI 0x478ba0 that character_step's `run` arg used to receive pre-resolved
 * (PORT-DEBT(char-run-trigger), now retired).  Each sim-tick: snapshot the prior
 * cmd_lr, reset it, detect a LEFT (ring id 2) / RIGHT (ring id 4) double-tap over
 * the manager's event ring (input_dash_double_tap, window = CHAR_DASH_WINDOW_MS),
 * and resolve the dash command with retail's self-sustain: a tap-tap STARTS the
 * dash, holding the direction KEEPS it (prev cmd_lr == 5/6), releasing ENDS it.
 * `axis_held[0..3]` are the UP/DOWN/LEFT/RIGHT held booleans; `now` is the same
 * GetTickCount() clock the ring records its timestamps in.  Returns the run flag
 * (1 while dashing) to pass straight into character_step.  Models the run_mode!=2
 * branch (the shipped default, runs/dash-window2); run_mode==2 "hold-to-run" is
 * PORT-DEBT(keybind-config).  The walk command (0x14854 == 1/2) + its press-window
 * warmup stay character_step's domain (PORT-DEBT(char-input-autorepeat)). */
int character_resolve_run(character *c, const struct input_mgr *m, uint32_t now,
                          const int *axis_held, uint32_t window);

/* Resolve the UP/DOWN POSE command (cmd[3]) from live input — the U/D half of
 * the char-AI 0x478ba0 (:248-259), sibling to character_resolve_run's L/R dash.
 * Each sim-tick: snapshot the prior cmd_pose, reset it, then set it from the
 * held axis + the event ring: DOWN held (axis_held[CHAR_AXIS_DOWN]) -> 10 if the
 * pose self-sustains (prev == 10) or a DOWN ring press (id 3) is aged in
 * [CHAR_POSE_WINDOW_LO, _HI]; UP held (CHAR_AXIS_UP) -> 0xb on the same terms
 * (ring id 1).  The UP block runs second (mirrors retail), so UP wins if both
 * are somehow held.  `now` is the GetTickCount() ms clock the ring stamps with.
 * Returns the pose command (0 / 10 / 0xb) and stores it in c->cmd_pose.
 *
 * Models the primary trigger (ring window + self-sustain); the 240 ms held-time
 * backstop (array_B) is PORT-DEBT(char-pose-holdtime) and the door-enter probe
 * (0x442a70 :260-285) is PORT-DEBT(char-up-door-probe).  The pose's *physics*
 * (the apply states 2/5/6) is a separate chip — this is the command layer only. */
int16_t character_resolve_pose(character *c, const struct input_mgr *m,
                               uint32_t now, const int *axis_held);

/* Resolve the SWORD unsheathe/sheathe TOGGLE from live input — the freeroam Z
 * key (USER ground truth, ckpt 155/159: Z draws/sheathes; sword2.osr).  Each
 * sim-tick, consume a fresh ring-9 (INPUT_RING_SWORD) press via input_poll_consume
 * (the 100 ms consume-on-read window = exactly one toggle per press, no repeat).
 *
 * A press does NOT flip c->sword_out immediately — it QUEUES the toggle, which
 * engages CHAR_SWORD_DRAW_STARTUP ticks later (see that constant): retail's Z
 * routes through a queued context-action (478ba0 -> cmd[5]/0xd2 -> the 442a70/
 * 45a300 action FSM) that re-installs the sword-out form only after a startup, so
 * res 0x571 fr96 first renders ~3-4 ticks after the press while Arche holds the
 * sword-IN idle.  The draw/sheathe ANIMATION is the clip layer's job (arche_sword
 * _clip watches sword_out for the edge); the bank swap (freeroam_step) also keys
 * off sword_out, so deferring sword_out shifts both together.  `now` is the
 * GetTickCount() ms clock the ring stamps with.  Returns the (current) sword_out.
 *
 * No quest gate (ckpt 159: Z draws on a fresh new game; the ckpt-155 "weapon+0xd4
 * case-8 gate" was a proxy artifact, retired).  Retail's id-9 also drives the
 * discrete sword-OUT attack (cmd4=0xf); that is the attack layer's concern, not
 * this toggle. */
int16_t character_resolve_sword(character *c, struct input_mgr *m, uint32_t now);

/* Resolve the sword-OUT ATTACK swing from live input — the cmd[4] half of the
 * char-AI 0x478ba0 (:296-303), sibling to character_resolve_pose/_run/_sword.
 * Each sim-tick: if a swing is active, advance attack_timer and clear `attacking`
 * when it reaches the kind's duration (the swing must complete before the next can
 * start — retail's +0x68 mid-swing lock).  Otherwise, if X is held (axis_held
 * [CHAR_AXIS_ATTACK] = the +0x128 attack level) AND sword_out AND grounded AND
 * >= CHAR_ATTACK_REFRACTORY_MS since the last swing, START a swing: set attacking,
 * reset attack_timer, latch attack_kind (chip 2a = CHAR_ATTACK_NEUTRAL), stamp
 * attack_last_ms.  `now` is the GetTickCount() ms clock (the 200 ms refractory).
 * The swing's MOVEMENT (stationary for neutral) is character_step's job (it brakes
 * vel to 0 while `attacking`); the swing ANIM is the clip layer's (arche_sword_clip
 * plays the attack clip while `attacking`).  Returns the new `attacking`.
 *
 * Models the trigger + the swing-complete lock; the +0x66 charge meter / combo
 * scaling + the HITBOX/damage are gameplay = PORT-DEBT(sword-attack-gameplay).
 * Directional swings (held dir at trigger) + their forward lunge = chip 2b. */
int16_t character_resolve_attack(character *c, const struct input_mgr *m,
                                 uint32_t now, const int *axis_held);

#endif /* OSS_CHARACTER_H */
