/*
 * char_physics.{c,h} — the character MOVEMENT step FUN_00467bb0 (3.3 KB): the shared
 * physics every character type's handler funnels into from the integrator 0x442a70.
 * Faithful on the retail layout (world_layout.h); rhost-verified inside retail
 * (`--hook shadow:char_physics`, full state).
 *
 * One call advances one body one tick from its command block (wl_cmd):
 *   - the body STATE machine (+0x38/+0x3a/+0x3c): stand 0 / move 1 (with the brake
 *     phase) / crouch 2 / jump 3 (windup -> impulse -> rise -> apex) / air 4 (land
 *     phase) / look up 5 / slide 6;
 *   - gravity: + gravity per tick up to the fall cap (a body fall_cap override, a
 *     third in water, a float boost over region-D floors), the rise gravities while
 *     a jump is held / released, the slide constants;
 *   - the vertical move (the entity mover 0x54e5c0, or the bounded mode-3 clamp);
 *   - horizontal: walk / run / air-control accel, the (negative) brake, reversals
 *     (decelerate, then the turn action or a facing flip), the boosts, then the
 *     entity mover 0x54db10 (step-down unless probing a non-player).
 *
 *   FUN_00467bb0  char_physics_step
 *   FUN_004385e0  char_part_top
 */
#ifndef OSS_CHAR_PHYSICS_H
#define OSS_CHAR_PHYSICS_H

#include <stdint.h>

#include "world_layout.h"

/* FUN_00467bb0 (thiscall self): advance `b` (a scratch or the real body of part
 * `part`) one tick under `cmd`.  `probe` = the integrator's probe mode (non-zero
 * when simulating ahead, 0x442a70's a5), `ctl` = its control override (0 normal,
 * 1 keep a jump, 2/3 freeze the state machine), `grounded` = b->grounded.
 * Returns retail's eax (the horizontal mover's result in modes 0..2). */
int char_physics_step(wl_char *self, const wl_cmd *cmd, wl_body *b, int32_t probe,
                      int32_t ctl, int32_t part, int32_t grounded);

/* FUN_004385e0: part `part`'s collision top (y + margin), part 0 when out of range. */
int32_t char_part_top(const wl_char *self, uint16_t part);

#endif /* OSS_CHAR_PHYSICS_H */
