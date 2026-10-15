/*
 * room_load.{c,h} — the ROOM SETUP 0x586010 that builds a room and runs it (retail layout;
 * rhost-verified inside retail: the setup in REPLACE by the corpus hash, its helpers per call).
 *
 * 0x586010 (the room loop object's method; the scene loop 0x59f2c0 calls it per room): resets
 * the room loop (+0x14..+0xcc, the camera leads -30000 / -27200, the BGM pin, the zone state),
 * deletes the old WORLD (0x54bfb0) and makes a new one (0x4017d0) whose +0x1038 = the room's
 * record, +0x103c = the area table, +0x2780 / +0x27b0 / +0x27b4 / +0x105c = the caller's
 * arguments; a 16-byte pool head at room loop +0xe8 (32 records of 0x84); the effects-volume
 * fader (+0x1040: 10 / 120 / 1200, volume 1000); the rooms (+0x4070..+0x4083 and every slot's
 * character cleared; world+0x2784 = the first room).  The entry GATE (the caller's id) is looked
 * up in the room record's zone table (+0x1c, 20 x 12 B: u16 id, +4 the area) -> the area table:
 * an area of another map (+4) that is not 999999 sets room loop +0x24.  The joysticks are
 * (re)created when missing (0x5ba120 / 0x5ba290).  Every entity band gets fresh body arrays
 * (0x58cf60: 64 bodies for the shots / characters / actors, 1 for the particles / band 96 /
 * +0x2560; the characters also drop their links, 0x58d020).  The view's 27 DRAW POOLS (+0x54,
 * 8-byte records: count, capacity, buffer of 60-byte nodes) are freed (0x587d50) and rebuilt with
 * fixed capacities.  The room-start SCRIPT runs (mode 7); a room change it asks for fades the
 * timed sounds out (0x587ce0, 400 a tick) and releases load group 4 (0x43e3a0).  The MAP: the
 * resource (module sys+0x31c, or the area table's +0x54008) is loaded (0x587970) and decoded
 * into the grid (0x587e00: the room record's +0x110 / +0x10c, room loop +0xd4), its objects
 * populated (0x58d460), the grid refreshed (0x54c640), the resource freed.  Each live actor with
 * a handle found in the fill table (+0x1804, 12-byte records, count +0x7804; 0x587db0) gets
 * 0x41e600.  The map sprite (sys+0xcd8), unless already built (0x49a050), gets its PALETTE: kind
 * (sys+0x1b0c +4) 3 = a per-channel tint by the time of day 0x8a93fc (0x491740: 1000/1000/1000,
 * 1000/700/600, 300/300/400 for 2..5); else the descriptor's palette (+0xc) or the sprite's own
 * (0x4178e0, through the swap table +0x8 when set, 0x417bc0), kinds 1 / 2 tinted per entry by
 * 0x4182d0 (time of day 0: 100/100/100, 1: 100/80/70, 2-3: 70/70/80, 4: 60/60/75, 5: 80/80/85;
 * kind 2 at 3: 90/90/90), gamma-mapped (0x8a9410) when the gamma is on, copied in (0x491770);
 * then 0x418470 + the parallax grades 0x8c / 0x8d (0x417c40).  The view: the map size off the
 * grid, the viewport 64000 x 48000, both shake blocks cleared (0x587d30).  The GATE ACTOR: the
 * first live actor with a zone kind and the gate's zone id (any, for gate 0; else the first with
 * a zone kind).  Every live map NPC (0x58cfe0 / 0x58cfc0) is dropped (0x4467d0) unless the
 * caller's keep list (+4 ids, u16 count +0x84) has it, or (no list) a roll of rand()*1000/0x8000
 * %100 <= its presence class (0: 70, 1 / 9: 100, 2: 25) keeps it — and a kept one (not class 9)
 * standing near the gate (24000 x 16000, 68000 x 24000 when `wide`) is dropped too.  The party:
 * each room's active slots get 0x58d060 + the member (0x58e170), the formation (0x41dc90), the
 * input flush, the partner's view record (0x58d210).  world+0x27b4 == 1: world+0x27ac = the wait
 * record, its +0x6c / +0x70 = the left / bottom + 1 of the actor with its zone id (+0x68).  The
 * camera snap (0x58e6a0), the encounter state (quiet), then the room's start EVENT
 * (0x40b8f0 mode 0): 1..3 / 6 end the setup at once; else the ROOM LOOP runs (0x58f360) until a
 * result ends it — 1: the zone's script (room+0x4024 / +0x4028, mode 1; 11 runs the room again,
 * 1 / 2 / 6 replace the result); 2 / 6 / 10 end; 4: the mode-5 script (6 = result 6); 5: the BGM
 * change 27 (0x40a5d0); else the room runs again (the loading log goes quiet after the first
 * run).  At the end: 0x58e330 (room+0x40ac), 0x56dc20, and for results 1 / 2 with no wait
 * record the scene runner stops (0x41a890 10020) and is deleted.
 *
 *   FUN_00586010  load_room            FUN_0058cf60  entity_bodies_alloc
 *   FUN_0058d020  char_links_clear     FUN_00587d50  view_pools_free
 *   FUN_00587d30  shake_clear          FUN_0043c9b0  volume_fade
 *   FUN_0058cfe0  char_map_id          FUN_0058cfc0  char_map_npc
 *   FUN_0058d060  pslot_status_clear   FUN_004022d0  gate_actor
 *   FUN_0049a050  sprite_slot_ready    FUN_00491740  sprite_slot_tint
 *   FUN_00587ce0  timed_sounds_fade    FUN_00587db0  fill_lookup
 *   FUN_0054bf90  world_leader         FUN_0058e6a0  camera_snap
 *   FUN_0058d0b0  zones_reset          FUN_0058d3b0  quest_state
 *   FUN_0058d210  view_record_init     FUN_0058e330  room_state_save
 *   FUN_005878a0  map_res_free         FUN_0059db70  map_res_delete
 *   FUN_00562d50  pool_head_delete
 *   FUN_0041dc90  room_formation       FUN_0041e020  char_follow_set
 *   FUN_004182d0  palette_tint         FUN_00587970  map_res_load
 *   FUN_00587e00  map_decode_room      (the grid: map_decode.c)
 */
#ifndef OSS_ROOM_LOAD_H
#define OSS_ROOM_LOAD_H

#include <stdint.h>

#include "world_layout.h"

struct camera_view;

/* 0x586010 (thiscall room loop, 15 stack arguments): the room's result (0x58f360's, or the
 * start event's 1..3 / 6) */
int load_room(wl_roomloop *rl, wl_room *rooms, uint16_t nrooms, uint8_t *areas, uint8_t *rinfo, int32_t gate,
              int32_t a7, const uint8_t *keep, const uint8_t *fill, int32_t fd4, int32_t wide, int32_t f2c,
              int32_t f27b4, uint8_t *wait, int32_t f2780, int32_t f105c);
/* 0x58cf60 (thiscall entity): a fresh array of n bodies (the old one freed), not alive */
void entity_bodies_alloc(wl_entity *e, uint16_t n);
/* 0x58d020 (thiscall character): drop the linked entities, the carrier / held character */
void char_links_clear(wl_char *c);
/* 0x587d50 (thiscall view): free the draw pools (+0x54) and their table */
void view_pools_free(void *view);
/* 0x587d30 (stdcall): clear a shake block (+0x14 kept) */
void shake_clear(void *shake);
/* 0x43c9b0 (thiscall fader): one fade step (state +0x20: 1 mute, 2 up, 3 down by +0x24 * 20 /
 * 1000 to 1000 / 0, then idle); `hold` != 0 = no step */
void volume_fade(uint8_t *f, int32_t hold);
/* 0x58cfe0 (thiscall character): its map record id (+0x1fc) while alive, standing, not busy
 * and placed by the map (+0x1f8 == 1); else 0 */
int32_t char_map_id(const wl_char *c);
/* 0x58cfc0 (thiscall character): placed by the map (+0x1f8 == 1) as faction 2 */
int char_map_npc(const wl_char *c);
/* 0x58d060 (thiscall slot): clear the slot's +0xe58 words where 0x628768's row is set */
void pslot_status_clear(wl_pslot *s);
/* 0x4022d0 (stdcall): the first live actor with a zone kind and zone id `id` (any for 0) */
wl_actor *gate_actor(int32_t id);
/* 0x49a050 (thiscall sprite slot): its surface is built */
int sprite_slot_ready(const void *slot);
/* 0x491740 (thiscall sprite slot): the per-channel tint (per mille) + the gamma LUT */
void sprite_slot_tint(void *slot, int32_t r, int32_t g, int32_t b, const uint8_t *lut);
/* 0x587ce0 (thiscall timed-sound list): every sound still playing fades out at `rate` a tick */
void timed_sounds_fade(void *list, int32_t rate);
/* 0x587db0 (thiscall fill table): *out = the value for `id`; 0 when absent */
int fill_lookup(const uint8_t *tbl, int32_t id, int32_t *out);

/* 0x54bf90 (thiscall world): the leader's character (NULL without a room / leader) */
wl_char *world_leader(const wl_world *w);
/* 0x58e6a0 (thiscall room loop): aim the view at the leader + the camera leads (clamped to the
 * map; follow cap 800, the far boost on), the listener on the leader; `snap` != 0 = jump there
 * (returns the view, else NULL) */
struct camera_view *camera_snap(wl_roomloop *rl, int32_t snap);
/* 0x58d0b0 (thiscall zone state): clear it; `cur` = the first live zone actor (not in its done
 * state) whose part 0 overlaps the leader's */
void zones_reset(wl_zonestate *z, const wl_char *ld);
/* 0x58d3b0 (thiscall room): a quest record's state against the room's flags: 0 = its prerequisite
 * (+0x8) below +0xc, 2 = its own flag (+0) set (*out 73), else 1 (*out 72) */
int quest_state(const wl_room *room, const uint8_t *rec, uint16_t *out);
/* 0x58d210 (thiscall view record): set the record up for `room` (+0x10 = v); +0xc = the quests
 * open (state 1) in the town maps 1010 / 1020 */
void view_record_init(uint8_t *r, wl_room *room, int32_t v);
/* 0x58e330 (stdcall room state): save every live actor's switch state (+0xc) by its handle */
void room_state_save(uint8_t *t);
/* 0x5878a0 (thiscall map resource): free its buffers */
void map_res_free(uint8_t *m);
/* 0x59db70 (thiscall map resource): + itself when flags & 1 */
void *map_res_delete(uint8_t *m, int32_t flags);
/* 0x562d50 (thiscall pool head): free its records (their +0x48 objects through 0x40e0c0), + itself
 * when flags & 1 */
void *pool_head_delete(uint8_t *h, int32_t flags);

/* 0x41dc90 (thiscall room): each active member's formation parameters (char +0x14828, by the
 * formation kind: the member's slot +0xa00 / room +0x4054) and follow set-up: the leader (or every
 * member without one) mode 2; others by the follow mode (slot +0x9fc / room +0x4050): 1 behind the
 * leader, 2 in a line (4000 per order step), else 0x41e020 mode 3; 4800 per member placed ahead */
void room_formation(wl_room *r);
/* 0x41e020 (thiscall character): follow `leader` in `mode` at the given distances */
void char_follow_set(wl_char *c, int32_t mode, wl_char *leader, int32_t side_dist, int32_t follow_dist);
/* 0x4182d0 (stdcall): dst rgb = src rgb (or their mean, `gray`) x r / g / b percent, clamped 0..255 */
void palette_tint(uint8_t *dst, const uint8_t *src, int32_t r, int32_t g, int32_t b, int32_t gray);
/* 0x587970 (thiscall map resource): read DATA resource `id` of `hmod` into it (the 0x34-byte head,
 * the cells, the 60-byte object records and their four arrays); 0 when the resource is missing */
int map_res_load(uint8_t *m, void *hmod, int32_t id);

/* 0x587e00 (thiscall grid): decode the map resource object into the grid (map_decode.c) + the
 * prologue's engine-side effects: the tileset's palette-remap tables (5..8 / 9..13; 5 / 6 / 8 also
 * world+0x27a4 = 136 / 137 / 138), the time of day 0x8a93fc (0x56df10 on a change), the tileset
 * banks sys+0x302 / +0x304 / +0x306 (the old one's frames flushed) */
void map_decode_room(uint8_t *grid, uint8_t *mres, int32_t type, int32_t scene, int32_t tileset);

#endif /* OSS_ROOM_LOAD_H */
