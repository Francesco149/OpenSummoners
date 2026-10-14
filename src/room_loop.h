/*
 * room_loop.{c,h} — the per-tick pieces of the ROOM LOOP 0x58f360 (retail layout;
 * rhost-verified inside retail).  The loop itself (its pacing machine, render, Flip) is
 * still retail's; these are the calls it makes each update:
 *
 *   input (per room, 0x468a20): each input manager's held axes are cleared, refilled by the
 *     keyboard / joystick producers (0x46a880 / 0x468b10, slots), and +0x140 stamps the tick
 *     an axis went down (0 while up);
 *   every live character: the support probe + its AI (0x590410: the player AI 0x478ba0 for
 *     mode 0, the behaviour AI 0x47b990 for mode 1 or a collision-off character);
 *   every live actor (0x58ffd0): the cooldown, the faction-3 proximity check every 16 ticks,
 *     the actor update, the water splash, the effect records, the extra parts, the sound
 *     channels — the same body the tick driver 0x46cd70 inlines;
 *   the map's tile-animation phase (0x58ff50); the party check (0x59ca10: 4 = no member
 *     standing); the camera lead (0x59d3f0: toward the partner or the facing, 1/40 a tick);
 *   the timed sounds (0x43e140, sys+0xaa8: start, play for the duration, fade out).
 *
 *   FUN_00468a20  room_input_update   FUN_00590410  room_char_step
 *   FUN_0058ffd0  room_actor_step     FUN_0058ff50  room_grid_anim
 *   FUN_0059ca10  room_party_down     FUN_0059d3f0  room_lead
 *   FUN_0043e140  timed_sounds_tick
 */
#ifndef OSS_ROOM_LOOP_H
#define OSS_ROOM_LOOP_H

#include <stdint.h>

#include "world_layout.h"

/* 0x468a20 (ecx = room) */
void room_input_update(wl_room *r);
/* 0x590410 (fastcall c, world; `cut`): the support probe's result, or the AI's */
int room_char_step(wl_char *c, int32_t cut);
/* 0x58ffd0 (thiscall actor) */
void room_actor_step(wl_actor *a);
/* 0x58ff50 (thiscall grid) */
void room_grid_anim(uint8_t *grid);
/* 0x59ca10: 0 while a party member of `r` stands (live, body on, not busy), else 4 */
int room_party_down(const wl_room *r);
/* 0x59d3f0 (thiscall room loop): ease rl->lead toward the target of `mode` (1: the gap to
 * `partner`, 2: the facing of `c`) */
void room_lead(wl_roomloop *rl, const wl_char *c, const wl_char *partner, int32_t mode);
/* 0x43e140 (thiscall sys) */
void timed_sounds_tick(void *sys);

#endif /* OSS_ROOM_LOOP_H */
