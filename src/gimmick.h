/*
 * gimmick.{c,h} — the room loop's GIMMICK pass (retail layout; rhost-verified inside retail).
 *
 * Once per tick the room loop 0x58f360 runs 0x59afd0 over the 128 world actors: each live
 * one gets 0x59b170, which drives the SWITCH actors by code — a plate the leader stands on
 * (70130 on/off, 70100 one-shot with sound 12), a weight plate (70215: riders), a lever a
 * rider pulls off (70600), an area trigger (70800: any character, or a linked actor's
 * box), a follower of another switch (70810, optionally inverted by +0x2cc), a timed plate
 * (70830: characters of an id range, two ticks of debounce) — and then a pending hit
 * (+0x294: break 1/2, toggle 3, switch on 4).  Switching goes through 0x40afe0, which runs
 * the per-code effect (bridges 70400 / 70450 break into dust, 70820 turns solid, 70850
 * gathers the party, 70220 fires shots / warps, 70251.. glow), sets the step record's clip
 * and sound, runs the actor's event script (0x40b8f0, mode 3) and propagates to every
 * actor whose handle is its link (0x40be30, recursively).  A script's non-zero result
 * other than 1 / 2 / 6 flushes the held input of every party member.
 *
 *   FUN_0059afd0  gimmick_pass        FUN_0059b170  gimmick_step
 *   FUN_0040afe0  switch_set          FUN_0040be30  switch_link
 *   FUN_0040c300  actor_unlist_all    FUN_00408100  switch_glow
 *   FUN_004804b0  entity_rect_hit     FUN_00556610  chars_in_rect_count
 *   FUN_005567a0  char_in_rect        FUN_005568e0  char_riding
 *   FUN_00556de0  particle_find       FUN_0040be00  world_next_serial
 *   FUN_0040ac10  actor_shot          FUN_0054ecf0  warp_request
 *   FUN_0054efd0  party_gather
 * (+ actor_switch_off: the switch-off 0x59b170 and the actor update inline.)
 *
 * The TOUCH pass 0x59bdd0 runs when the player presses up / talk (key 37; key 38 =
 * `pressed`): not with a hostile character within 20000 x 16000, not mid-action, not
 * faster than 24000, only on the ground (and not on a kind-2 platform with +0x2f8).  Two
 * probe rects ahead of the body (4000 wide 800 ahead, 3200 wide from 800 inside, from
 * 1600 above the top to the feet): the nearest entity of the +0x2790 list whose box
 * meets its rect (+0x1e4: 1 the inner, 2 the outer) runs its script (mode 2) — 0 tries
 * the next nearest, anything else (24 = a room change) flushes the input and returns.
 * With none, the first actor touched whose 0x59c240 acts: 70216 (on, off again
 * unless +0x28c is 1..2), 70217 (locked: 0x54ed80 uses a key), 70220 / 70353.. (on).
 *
 *   FUN_0059bdd0  touch_pass          FUN_0059c240  touch_actor
 *   FUN_00411560  room_input_flush    FUN_004812c0  body_probe_rect
 *   FUN_0047b7c0  entity_near
 */
#ifndef OSS_GIMMICK_H
#define OSS_GIMMICK_H

#include <stdint.h>

#include "world_layout.h"

/* 0x59afd0 (thiscall room loop): the pass; 1 / 2 / 6 = a script's result to return */
int gimmick_pass(wl_roomloop *rl, wl_char *player);
/* 0x59b170 (thiscall actor): one actor's step against the player character */
int gimmick_step(wl_actor *a, wl_char *player);
/* 0x40afe0: switch `a` to state `v`; `quiet` != 0 skips its script + links.  A script's
 * result (6 wins) or 0. */
int switch_set(wl_actor *a, int32_t v, int32_t quiet);
/* 0x40be30: switch the actors whose handle is a's link (+0x29c inverts; +0x288 = all) */
int switch_link(wl_actor *a, int32_t v, int32_t quiet);
/* the switch-off both 0x59b170 and the actor update 0x54f980 inline: 1 = switched off */
int actor_switch_off(wl_actor *a);
/* 0x40c300: off the hittable list (+0x2798) and every entity list */
void actor_unlist_all(wl_actor *a);
/* 0x408100: the glow of codes 70251 / 70252 / 70371 (on when off; `burst` sparks when on) */
void switch_glow(wl_actor *a, int32_t v, int32_t burst);
/* 0x4804b0: does part `part` of `e` (0 when out of range) overlap rect {x, y, w, h}? */
int entity_rect_hit(const wl_entity *e, const int32_t *r, uint16_t part);
/* 0x556610: live faction-2 characters (id within lo..hi, 0 = open) overlapping `r` */
uint16_t chars_in_rect_count(uint16_t part, const int32_t *r, uint32_t lo, uint32_t hi);
/* 0x5567a0: a live weighing character overlapping `r` (`leader_only`: the room's leader) */
int char_in_rect(uint16_t part, const int32_t *r, int32_t leader_only);
/* 0x5568e0: the (weighing, kind 1) rider standing on `a`; *state = its body state */
wl_entity *char_riding(const wl_actor *a, int32_t leader_only, uint16_t *state);
/* 0x556de0: the first live particle owned by `owner` (of `code`, 0 = any) */
wl_entity *particle_find(const wl_entity *owner, int32_t code);
/* 0x40be00: bump the world action serial (mod 0x1000000); returns the old value + 1 */
uint32_t world_next_serial(wl_world *w);
/* 0x40ac10: one shot (100040) from body `b` at power +0x2e0[1] * power / 1000 */
int actor_shot(wl_actor *a, wl_body *b, int32_t a3, int32_t power);
/* 0x54ecf0: request a warp to (+0x2d0, +0x2d4) (`kind` 2 always; else only in free play) */
void warp_request(wl_actor *a, int32_t kind);
/* 0x54efd0: put every other live party member on the ground at body `at`, reset */
void party_gather(const wl_body *at);

/* 0x59bdd0 (thiscall room loop): the player's touch; a script's result (6 wins) or 0 */
int touch_pass(wl_roomloop *rl, wl_char *player, int32_t pressed);
/* 0x59c240: actor `a` touched; *acted = 1 when it switched (the caller flushes input) */
int touch_actor(wl_actor *a, int32_t *acted);
/* 0x411560 (thiscall room): drop the held input of every active party slot (not 100000104) */
void room_input_flush(wl_room *r);
/* 0x4812c0: the probe rect `p` {ahead, width, dy, height} in front of body `b` -> out {x,y,w,h} */
void body_probe_rect(const wl_body *b, const int32_t *p, int32_t *out, int32_t facing);
/* 0x47b7c0: are the centres of `self` and `o` (part 0) within rx / ry (+dy) of each other? */
int entity_near(const wl_entity *self, int32_t rx, int32_t ry, const wl_entity *o, int32_t dy);

#endif /* OSS_GIMMICK_H */
