/*
 * room_loop.{c,h} — the ROOM LOOP 0x58f360 and its per-tick pieces (retail layout;
 * rhost-verified inside retail: the pieces per call, the loop in REPLACE by the corpus hash).
 *
 * The loop runs a room until a result ends it.  Its PACING machine (GetTickCount, 16 ms
 * steps): state 0 pumps once; 1 renders (0x48c150, + "Pause" blinking every 30 updates
 * while paused) and Flips — then, after pumping, budget += now - last (capped 100) and once
 * it exceeds 16 the next pass is 2; 2 runs ONE update — then, if the time since exceeds the
 * budget, budget = 0 and back to 1, else budget -= 16 (back to 1 at <= 16).  An update:
 * timed sounds, the input re-poll + per-room update (ring id 19 sets +0x34); a waiting event
 * (world+0x27ac) ends on any fresh key or its tick budget (result 6); else the keys (unless
 * +0x28): the leader swap (id 20, a 40-tick hold), pause (18), the camera leads (42 / 43 /
 * 40 / 41 / 11), exit (34 -> 6), the party / item menus (21 / 22 / 23), the main menu (16),
 * the touch pass (37 / 38); results 1 / 2 / 4 / 5 / 6 end the room.  Unless paused: the
 * hitstop (+0x3c: every other update skipped for +0x3a), the world tick (+0x276c), every
 * character's step, the region flag, +0x279c, the apply pass, shots, particles, band 96,
 * actors, the tile phase; then a dead leader is swapped, the camera lead, the party-down /
 * warp / pickups / gimmick / scene-runner / encounter (every 5th) / zone passes (a zone
 * starts its scene: result 1), the per-room 0x5908a0, the camera snap + follow, 0x499ab0.
 *
 * Its per-tick calls:
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
 *   the timed sounds (0x43e140, sys+0xaa8: start, play for the duration, fade out);
 *   every 5th update the ENCOUNTER pass (0x58e780): world+0x2770 = 0 roam, 1 / 2 battle (a
 *     hostile near a party member; 2 when the threat score is high), 3 a strong foe while
 *     the leader's status[15] runs; world+0x2774 = a foe is near; the battle lingers 50 ticks
 *     after the last foe; then the room BGM (0x40d9b0) is restored — at once after a mode
 *     change (fade 20 / 200), or after 10 ticks when it differs.  Threat score (0x58eeb0 /
 *     0x58f0b0): per hostile in range by its level sum (party +0xd8 / +0xe0 / +0xe4) against the
 *     reference's: -4 -> 4, -3..-2 -> 6, -1..1 -> 10, 2 -> 15, >= 3 -> 40.
 *
 *   FUN_00468a20  room_input_update   FUN_00590410  room_char_step
 *   FUN_0058ffd0  room_actor_step     FUN_0058ff50  room_grid_anim
 *   FUN_0059ca10  room_party_down     FUN_0059d3f0  room_lead
 *   FUN_0043e140  timed_sounds_tick
 *   FUN_0058e780  encounter_tick      FUN_0058eeb0  foes_near
 *   FUN_0058f0b0  party_foes_near     FUN_00412db0  pslot_ok
 *   FUN_0040d9b0  bgm_pick
 *   FUN_0058f360  room_loop_run       FUN_0049be60  input_repoll_flush
 *   FUN_004115e0  menu_ctx_clear      FUN_0058d090  room_leader
 *   FUN_0041a140  room_views_mark
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
/* 0x58e780 (thiscall room loop): `quiet` != 0 = no BGM change */
void encounter_tick(wl_roomloop *rl, wl_room *room, int32_t quiet);
/* 0x58eeb0 (thiscall c): a live hostile (to c) — within rx / ry of c when `ranged`, of the
 * player AI / engaged when `ai_only` — exists; *score = their threat against `ref` (c) */
int foes_near(const wl_char *c, int32_t ranged, int32_t rx, int32_t ry, uint16_t *score, int32_t ai_only,
              const wl_char *ref, int32_t lvl);
/* 0x58f0b0: a party member with hostiles within rx / ry scoring >= `min`; *best = the top */
int party_foes_near(const wl_room *room, const wl_char *ref, uint16_t *best, int32_t rx, int32_t ry, int32_t ai_only,
                    uint16_t min, int32_t lvl);
/* 0x412db0 (thiscall slot): active (`any`: also id 100000104), or kind 2 when `inactive` */
int pslot_ok(const wl_pslot *s, int32_t any, int32_t inactive);
/* 0x40d9b0 (thiscall room loop): the BGM the room wants now */
uint16_t bgm_pick(const wl_roomloop *rl);
/* 0x58f360 (thiscall room loop): run the room; its result (1 / 2 / 4 / 5 / 6, a zone's 1) */
int room_loop_run(wl_roomloop *rl);
/* 0x49be60 (thiscall room loop): re-poll DirectInput, drop every room's held input */
void input_repoll_flush(wl_roomloop *rl);
/* 0x4115e0: clear the main menu's context fields */
void menu_ctx_clear(void *ctx);
/* 0x41a140 (thiscall room loop): both views' +0 = v */
void room_views_mark(wl_roomloop *rl, int32_t v);
/* 0x58d090 (thiscall room): the leader's character (NULL without a leader) */
wl_char *room_leader(const wl_room *r);

#endif /* OSS_ROOM_LOOP_H */
