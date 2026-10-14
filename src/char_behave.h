/*
 * char_behave.{c,h} — the BEHAVIOUR AI FUN_0047b990 (7.5 KB): what a non-player
 * character (char+0x200 counted down to 1 by the tick driver: party members, townsfolk,
 * monsters) wants this tick, as a command block for the integrator.  Faithful on the
 * retail layout; rhost-verified inside retail (`--hook shadow:char_behave`, full state).
 *
 * Two paths:
 *   - a cutscene tick with +0x15ba8 == 1: the RANDOM WALK — 5 candidate directions x an
 *     ordered action list (walk, crouch, look up, jump, the chase / ranged skills), each
 *     simulated (0x47d790 / npc_try_cmd 0x4412d0); the first accepted command wins.
 *   - otherwise (cutscene ticks only with +0xc850): the cooldowns, the side of the
 *     leader to keep to (ally strength left / right, re-picked every 60 ticks), the
 *     re-think pause, target / ally tracking (0x47e330, engage / disengage by distance),
 *     the attack choice while acting (0x47f9d0 / 0x47ecd0 -> states 3 / 6), the per-code
 *     extras (Arche's partners 50010, leashed summons, the 58010 pacer), then the state
 *     machine over the behaviour helpers (0x482ac0 .. 0x485990, engine_calls slots),
 *     and the leader-follow run boost (0x4854e0).
 *
 *   FUN_0047b990  char_behave
 */
#ifndef OSS_CHAR_BEHAVE_H
#define OSS_CHAR_BEHAVE_H

#include <stdint.h>

#include "world_layout.h"

/* `cut` = a cutscene tick (the tick driver's argument), `b` = the live body, `gr` = its
 * support probe result.  Returns retail's eax. */
int char_behave(wl_char *self, int32_t cut, wl_body *b, int32_t gr);

#endif /* OSS_CHAR_BEHAVE_H */
