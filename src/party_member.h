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

#endif /* OSS_PARTY_MEMBER_H */
