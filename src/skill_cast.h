/*
 * skill_cast.{c,h} — the party characters' SKILL CASTS on the retail layout (rhost-verified inside retail): the
 * integrator 0x442a70 runs `skill_cast` for the party types (Arche, Sana, 50012) and other casters every tick with
 * a per-type clip table, before the type's own handler.  Its helpers: the charge (hold to power up), a skill's
 * stage machine, the MP payment, and the summons (spawn into the 32-char band, init by type, recall, the room
 * limits), the space-ahead probe.
 *
 * A FIRST PASS through tools/ida/decomp2c.py (IDA's control flow and local types kept), finished by hand.  Only
 * skill_cast / skill_charge / skill_stage / skill_mp_pay are reached by the corpus; the summon / space helpers
 * are unexercised.  Slots: the item / skill effect 0x5b3b00 (its chain 0x47ae00 0x48be60 0x5b28a0), the log
 * 0x408b90.
 *
 *   FUN_0045a300  skill_cast         FUN_00459f20  skill_charge       FUN_0045db20  skill_stage
 *   FUN_0045a2c0  skill_mp_pay       FUN_00456a50  summon_recall      FUN_00456b60  summon_room_ok
 *   FUN_0045e050  skill_space_ahead  FUN_0045e4c0  summon_init        FUN_0055ce70  summon_spawn
 */
#ifndef OSS_SKILL_CAST_H
#define OSS_SKILL_CAST_H

#include <stdint.h>

#include "world_layout.h"

int32_t skill_cast(wl_char *self, int32_t b, int32_t probe, int32_t ctl, int32_t cmd, int32_t grounded, int32_t *dt,
                   uint16_t *clips, int32_t prio);                                     /* 0x45a300 */
int32_t skill_charge(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t *a5, int32_t a6, uint16_t *a7,
                     uint16_t a8, int32_t a9);                                         /* 0x459f20 */
int32_t skill_stage(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t *a5, int32_t a6, uint16_t *a7,
                    uint16_t a8, int16_t a9, int32_t a10);                             /* 0x45db20 */
int32_t skill_mp_pay(wl_char *self, int32_t skill, int32_t probe, int32_t pay);       /* 0x45a2c0 */
int32_t summon_recall(int32_t owner, int32_t code);                                    /* 0x456a50 */
int32_t summon_room_ok(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5); /* 0x456b60 */
int32_t skill_space_ahead(int32_t *a1, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t *a6); /* 0x45e050 */
int32_t summon_init(wl_char *self, int32_t *a2);                                       /* 0x45e4c0 */
int32_t summon_spawn(int32_t a1, int32_t *a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7,
                     int32_t a8, int32_t a9);                                          /* 0x55ce70 */

#endif /* OSS_SKILL_CAST_H */
