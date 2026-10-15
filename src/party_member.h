/*
 * party_member.{c,h} — the PARTY MEMBER create / attach (retail layout; rhost-verified inside retail).
 *
 * At every room entry the room setup 0x586010 makes each active party slot's member: 0x58e170 takes
 * the next free of the world's 32 characters (round robin from room loop +0xcc), inits it
 * (0x41f200) at the gate entity's body-0 feet (anchor 3, +0x1f8 = 2) with the dramatist row's code
 * for the slot id (+0x9f0; else the slot's +4) and the slot id as the handle, and attaches it
 * (0x5b4530): slot +0x9f4 = it, its stat block = the slot's +0xa10, the pacing re-rolled by the
 * slot's +0xa04 kind, +0x1dc = the mode, the handle, the input (slot +0: the player's input
 * manager, else +0x200 = 1 = NPC brain), the AI reset, the ground probe, respawn on at body 0 (x,
 * bottom), Arche's party-mode look-up, +0x15900 = 40 (0 in two-room play, where Arche's codes
 * 50010..50013 also get the member pose 107 / 112 / 6000), the equipment palette (0x5b4a10) and the
 * skill-slot refresh (0x406ec0).
 *
 * The equipment palette: the base bank (track 0, unless it is already a clone 884..908) palette,
 * recoloured by every equipped item of the slot's room (+0x2014 records, 16 B, count u16 +0x4010:
 * owner id, item +0x201c with +0x78 set and a swap table +0x2b4); any recolour (or an already
 * cloned track) gives each live track a free clone bank (sys+0x18e0 descriptor +0x10 = taken;
 * 0x4179b0), frees its frames and installs the palette.  The skill refresh: every skill's mode
 * +0x2f8 = 1 when its +0x2fc is set, else the member record's (stat +0x148, 12 B: id, mode, u16
 * slot; count u16 +0x144); with `cycle`, the class stat +0x140 steps 1 -> 2 -> 3 -> 1 to one with a
 * usable skill and the class's slot cursor (u16 stat +0x138 + 2 x class) advances to the next
 * usable member skill (9 tries, mod 10; 0 when none).
 *
 *   FUN_0058e170  member_create        FUN_005b4530  member_attach
 *   FUN_004094d0  pslot_code           FUN_005b4a10  member_palette
 *   FUN_00406ec0  char_skills_refresh  FUN_00407320  char_skill_usable
 *   FUN_004071f0  member_skill_at
 *
 * The PARTY API (the story setups 0x4c5350 / 0x4e59a0 at a new game and the scripts): a slot
 * by id (0x413b20; status 1 = in the party, 2 = known but out), a slot's init from the stat
 * table (0x4c5e00 / 0x4cc820: + the code's skills 0x4c6160 and its starting items 0x4c6990 /
 * 0x415e20), the party levelled to the area's row (0x408dc0: 0x690b40 by room +0x401c: [1]
 * level, [2] the minimum +0xaec, [3] the gold cap +0x4020), heal (0x408d40), the leader
 * (0x4c6830: the input moves with it), join / leave (0x4c6550 / 0x4c63a0 / 0x4e6070: the
 * items they hold are re-equipped / handed back), the stat refresh from the equipment (0x413810
 * / 0x416570 + 0x413a00 per item), the item / class fit (0x416420).
 *
 *   FUN_00413b20  pslot_by_id          FUN_00409090  dramatist_row
 *   FUN_0041dc40  area_level_row       FUN_00408d40  pslot_heal
 *   FUN_00408dc0  room_party_level     FUN_004c5e00  pslot_init
 *   FUN_004c6160  member_skills_learn  FUN_004c6990  room_items_grant
 *   FUN_004c6830  room_leader_set      FUN_004c5350  room_story_setup
 *   FUN_004e59a0  room_story_setup2    FUN_00429d00  skill_row_find
 *   FUN_00416420  item_fits            FUN_004e6030  pslot_status_set
 *   FUN_004c6530  pslot_revive_mark    FUN_00413a00  item_stats_add
 *   FUN_00413810  room_stats_refresh   FUN_00416570  room_item_equip
 *   FUN_004e6070  room_party_dismiss   FUN_004c63a0  room_member_leave
 *   FUN_004c6550  room_member_join     FUN_004cc820  room_member_add
 *   FUN_00415e20  room_item_add
 */
#ifndef OSS_PARTY_MEMBER_H
#define OSS_PARTY_MEMBER_H

#include <stdint.h>

#include "world_layout.h"

/* 0x58e170 (thiscall room loop, 5 args; the first is unused) */
wl_char *member_create(wl_roomloop *rl, int32_t unused, const wl_entity *at, wl_pslot *s, int32_t facing, int32_t mode);
/* 0x5b4530 (thiscall char): returns 0x406ec0's */
int member_attach(wl_char *c, wl_pslot *s, int32_t mode);
/* 0x4094d0 (thiscall slot): the dramatist row's code for its id, else +4 */
int32_t pslot_code(const wl_pslot *s);
/* 0x5b4a10 (thiscall char) */
void member_palette(wl_char *c);
/* 0x406ec0 (thiscall char): the class slot index it settled on (0..2) */
int char_skills_refresh(wl_char *c, int32_t cycle);
/* 0x407320 (thiscall char): the party class gate (+0xc0) and the mode (+0x2f8; 3 not while probing,
 * 255 only with +0x1f0) */
int char_skill_usable(wl_char *c, const uint8_t *k, int32_t probing);
/* 0x4071f0 (stdcall): the member record of class `cls` in slot `slot` whose skill table row has
 * +0xa8 set, or NULL */
uint8_t *member_skill_at(const uint8_t *st, int32_t cls, uint16_t slot);

/* the party API (see the head) */
wl_pslot *pslot_by_id(wl_room *r, int32_t id, int32_t passive);                          /* 0x413b20 */
const uint8_t *dramatist_row(int32_t id);                                                /* 0x409090 */
const int32_t *area_level_row(int32_t key);                                              /* 0x41dc40 */
void pslot_heal(wl_pslot *s, int32_t full);                                              /* 0x408d40 */
void room_party_level(wl_room *r, int32_t quiet);                                        /* 0x408dc0 */
void pslot_init(wl_pslot *s, wl_room *r, int32_t code, int32_t level, int32_t id, int32_t status,
                int32_t order);                                                          /* 0x4c5e00 */
void member_skills_learn(wl_pslot *s, int32_t code, int32_t level);                      /* 0x4c6160 */
void room_items_grant(wl_room *r, int32_t id);                                           /* 0x4c6990 */
void room_leader_set(wl_room *r, wl_pslot *s);                                           /* 0x4c6830 */
void room_story_setup(int32_t *key);                                                     /* 0x4c5350 */
void room_story_setup2(int32_t *key);                                                    /* 0x4e59a0 */
const uint8_t *skill_row_find(int32_t id);                                               /* 0x429d00 */
int item_fits(const wl_pslot *s, const uint8_t *item);                                   /* 0x416420 */
void pslot_status_set(wl_pslot *s, int32_t in, void *ch);                                /* 0x4e6030 */
void pslot_revive_mark(wl_pslot *s);                                                     /* 0x4c6530 */
void item_stats_add(const uint8_t *item, uint8_t *st);                                   /* 0x413a00 */
void room_stats_refresh(wl_room *r, int32_t palette);                                    /* 0x413810 */
void room_item_equip(wl_room *r, uint16_t k, int32_t id, int32_t refresh);               /* 0x416570 */
void room_party_dismiss(wl_room *r);                                                     /* 0x4e6070 */
void room_member_leave(wl_room *r, wl_pslot *s, int32_t check);                          /* 0x4c63a0 */
wl_pslot *room_member_join(wl_room *r, int32_t id, wl_char *ch, int32_t mode);           /* 0x4c6550 */
wl_pslot *room_member_add(wl_room *r, uint16_t idx, int32_t code, int32_t level, int32_t id, int32_t order,
                          int32_t active);                                               /* 0x4cc820 */
int room_item_add(wl_room *r, int32_t item, uint16_t *idx, int32_t count, uint8_t flag, int32_t dry); /* 0x415e20 */

#endif /* OSS_PARTY_MEMBER_H */
