/*
 * script_engine.{c,h} — the SCRIPT ENGINE 0x40c380 (the event runner 0x40b8f0's body) + its
 * runner helpers (retail layout; rhost-verified inside retail).
 *
 * `ev` is the runner world+0x1054 (0xb2c bytes: +0x0 the room loop, +0x4 the actor, +0x8 the
 * room, +0xc unload, +0x10 the door id, +0x14 the mode; the scene command block +0x1c..+0xb28).
 * The chapter scripts run first: area 3 → 0x4e1d10, 1020 → 0x4e23d0, 1010 → 8 (flag 100100101 in
 * 0..500) then 28 (1000..2900); then 28 for every area (0x49d930 .. 0x4c45a0).  Each returns != 0
 * when it handled the event (the result is returned) and stops the run when it set unload.  With
 * none: by the mode — 0 the room arrival (the teleport notices, the party fade-in, the room BGM,
 * a 1 s wait, the area title on a map change), 1 a door ("The door is locked." for a locked one:
 * 11), 3 an actor switched (drawers / shelves / closets 0x407970, the chest 70220 0x4081d0, the
 * save statue 70100 with its BGM, six talk records → the leader faces the actor + 0x411610),
 * 5 0x40a340, 7 the time of day from flag 100100102.
 *
 *   FUN_0040c380  script_engine       FUN_0049cb60  script_begin      FUN_0049cd70  script_end
 *   FUN_0041e2f0  script_flag         FUN_00402f20  script_leader     FUN_00402060  script_party_cast
 *   FUN_00401f50  script_cast_add     FUN_00439680  script_wait       FUN_004afda0  script_none
 *   FUN_004081d0  item_find           FUN_00407970  furniture_search    FUN_00407500  item_give
 * (a map record's actor 0x556eb0 is actor_update.h's actor_by_handle)
 * The chest / furniture / storage talk are reached through their g_eng slots (h4081d0 / h407970 /
 * h411610) although ported: the slots are rhost's shadow-guard points (they run frame loops), and
 * dispatch to the port in replace mode — like the scene command run h439690.
 */
#ifndef OSS_SCRIPT_ENGINE_H
#define OSS_SCRIPT_ENGINE_H

#include <stdint.h>

#include "world_layout.h"

/* 0x40c380 (thiscall runner) */
int script_engine(wl_event *ev);
/* 0x4081d0 (thiscall runner): a find — `what` 1 / 2 / 3 = n copper / silver / gold coins, else an item
 * id (n: the count, 0 = the item's own); `a` (when set) sparkles first (item_sparkle, `sound`); `where`
 * the container's name ("treasure chest", "drawer"...: NULL = none).  The messages through the scene
 * command runner; the money / item go in unless full ("put back": *back = 1).  6 = the scene ended. */
int item_find(wl_event *ev, wl_entity *a, int32_t what, int32_t n, int32_t *back, const char *where, int32_t sound);
/* 0x407970 (thiscall runner): searching a piece of furniture `a` (its +0x2cc / +0x2d0 find) as a scene;
 * when the find is put back the furniture shuts again */
int furniture_search(wl_event *ev, wl_entity *a, const char *where);
/* 0x407500 (thiscall runner): a story gift — `what` 1 / 2 / 3 coins (n, 10 n, 100 n CP; nothing at the cap),
 * else item `what` (n: the count, 0 = its own; refused at 512 records / 48 normal items).  `say`: "You
 * obtained ..." (or the full message), then the scene mark +0x2e4.  *got = 1 when taken.  6 = the scene ended. */
int item_give(wl_event *ev, int32_t what, int32_t n, int32_t say, int32_t *got);
/* 0x49cb60 (thiscall runner): clear the scene command block, the characters' scene state */
void script_begin(wl_event *ev);
/* 0x49cd70 (thiscall runner): stop the voice track, clear the room loop's scene state */
void script_end(wl_event *ev);
/* 0x41e2f0 (thiscall runner): the room's flag `id` (0 when unset) */
int32_t script_flag(const wl_event *ev, int32_t id);
/* 0x402f20 (thiscall runner): the room leader's character (0 without a leader) */
wl_char *script_leader(const wl_event *ev);
/* 0x402060 (thiscall runner): every active party member's map actor joins the cast */
void script_party_cast(wl_event *ev);
/* 0x401f50 (thiscall runner): `actor` takes a cast slot (its own, else the first free) */
void script_cast_add(wl_event *ev, void *actor);
/* the cast record of `actor` in the runner's 32 (+0x2fc, 20 B {used, lead, actor, actor, 0}): its own,
 * else the first free one, (re)initialised; NULL when all are taken (inlined by 0x401f50 / 0x401e60 /
 * 0x402d40 / 0x405e80 / 0x406210) */
uint8_t *script_cast_rec(wl_event *ev, void *actor, int32_t lead);
/* a cast record's actor (+0xc; a NULL record faults as in retail) */
uint8_t *script_cast_actor(const uint8_t *rec);
/* a cast member's scene command reset to `type` (char +0x15b64: 2 face, 3 join, 4 the stance) */
void script_cast_cmd(uint8_t *a, int32_t type);
/* 0x439680 (thiscall runner): run the scene commands set up so far (0x439690); 6 = quit */
int script_wait(wl_event *ev);
/* 0x4afda0: the empty chapter script */
int script_none(void);

#endif /* OSS_SCRIPT_ENGINE_H */
