/*
 * room_tick.{c,h} — the room loop's per-tick party chores (retail layout; rhost-verified inside retail):
 * the warp countdown 0x59c750, the leader swap 0x59aa40, the pickups 0x59ca90 (+ their helpers).
 *
 *   FUN_0059c750  room_warp_tick      FUN_0059aa40  room_leader_next   FUN_0059acc0  char_lead_ok
 *   FUN_0059ad20  char_on_ground      FUN_0059ca90  room_pickups       FUN_0059ce60  pickup_touch
 *   FUN_00557060  item_sparkle        FUN_005908a0  room_party_tick    FUN_00590660  room_item_pick
 *   FUN_0049ce70  quest_count         FUN_0059a910  room_views_refresh FUN_0058ffc0  world_notice_reset
 */
#ifndef OSS_ROOM_TICK_H
#define OSS_ROOM_TICK_H

#include <stdint.h>

#include "world_layout.h"

/* 0x59c750 (thiscall room loop): the warp countdown (u16 +0x28: 160 → at 40 the screen dissolve of world
 * +0x1040, at 0 result 10); a warp request (+0x10) starts it — refused outside a warp-able room ("For some
 * reason ..."), always for a teleport trap (+0x90c == 2) — and fades the party out */
int room_warp_tick(wl_roomloop *rl, wl_room *room);
/* 0x59aa40 (thiscall room loop): the next able party member after `cur` (+0xa0c order) takes the lead
 * (`forced`: only another one; else `cur` must stand); the event zone under the new leader is re-armed */
int room_leader_next(wl_roomloop *rl, wl_room *room, int32_t forced, wl_pslot *cur);
/* 0x59acc0 (thiscall char): a free-moving (move mode 1..3) or grounded character */
int char_lead_ok(wl_char *c, uint16_t part);
/* 0x59ad20 (thiscall char): part `part` stands on something */
int char_on_ground(wl_char *c, uint16_t part);
/* 0x59ca90 (stdcall): each party member touching a live pickup (world+0x23e0, 96) takes it: gold (to the cap
 * +0x4020; full: "You have all the money ..." once a minute) or, for the leader, an item ("You got %s!"; bags
 * full: once per 10 s) */
void room_pickups(wl_room *room);
/* 0x59ce60 (thiscall entity): part `part` of `e` overlaps part `part` of `o` (a part past the count: 0) */
int pickup_touch(const wl_entity *e, const wl_entity *o, uint16_t part);
/* 0x557060 (thiscall entity): the item-get sparkle 100137 on `e` (+0x25c the item), with `sound` the pool
 * sys+0x6d0 cue */
wl_entity *item_sparkle(wl_entity *e, int32_t item, int32_t sound);

/* 0x590660 (thiscall room): for an item of kind `kind` (`a4` its parameter), the member it helps most and the
 * best usable inventory record (unequipped, usable, not combat-only in a scene); `hp_max` (per mille) skips a
 * member above it; -1 = none */
int room_item_pick(wl_room *room, wl_pslot **out, int32_t kind, int32_t a4, int32_t hp_max);
/* 0x5908a0 (thiscall room): the low-HP beep (below 30 %, once), the screen flash fading (+0x40e4), the queued
 * item use (u16 +0x4070: 1 the auto-heal picks, 2 the chosen one; 0x5b1f10 applies it) */
void room_party_tick(wl_room *room);

/* 0x49ce70 (thiscall room): in areas 1010 / 1020, the quest rows of 0x6b7ee8 (1212 B: +0 the done flag, +8 the
 * gate flag, +0xc its minimum) open in `room` — the gate reached and not done */
uint16_t quest_count(const wl_room *room);
/* 0x59a910 (thiscall room loop): both views' quest counts (+0x134; 0 without a room +0x130) */
void room_views_refresh(wl_roomloop *rl);
/* 0x58ffc0 (thiscall world): the bag-full / money-full notice stamps (+0x1028 / +0x102c) cleared */
int32_t *world_notice_reset(wl_world *w);

#endif /* OSS_ROOM_TICK_H */
