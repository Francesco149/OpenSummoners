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
    int32_t vel;       /* body+0x28 — signed horizontal velocity accumulator     */
    int16_t facing;    /* body+0x2c — 1 right / 3 left                           */
    int16_t cmd_dir;   /* the latched walk direction (+1 right / -1 left / 0 idle)*/
    int16_t warm;      /* held-tick counter for the press->latch warmup          */
    int16_t held_dir;  /* last tick's commanded direction (re-arms warm on change)*/
} character;

/* Initialise a character at a spawn worldX + facing (1 right / 3 left), at rest. */
void character_init(character *c, int32_t spawn_world_x, int facing);

/* One sim-tick of the controllable walk: `axis_held[0..3]` are the UP/DOWN/LEFT/
 * RIGHT held booleans (held_trace replay / the real producer 0x46a880).  Runs the
 * AI reduction (held axis -> latched walk direction) then the apply reduction (the
 * velocity ramp + the worldX commit + the facing flip).  Returns the worldX delta
 * applied this tick (dwx = vel/100), for the host validation. */
int32_t character_step(character *c, const int *axis_held);

#endif /* OSS_CHARACTER_H */
