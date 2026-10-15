/*
 * scene_cmd.{c,h} — the scene COMMAND builders the chapter scripts call on the runner (retail layout;
 * rhost-verified inside retail).  Each fills the runner's command block (+0x20 the command kind, the
 * fields it reads) or a cast member's command (char +0x15b64), then the script runs it with 0x439680.
 *
 *   FUN_0049d6e0  scene_msg            FUN_004a5ee0  scene_msg_rec        FUN_00401e60  scene_face
 *   FUN_004a5e20  scene_face_rec       FUN_00402d40  scene_party_stance   FUN_00406210  scene_talk_face
 *   FUN_0041d190  script_flag_set      FUN_004a5f50  scene_camera         FUN_00401d40  scene_next_event
 *   FUN_004a5f30  scene_pause          FUN_004a5f90  scene_mark           FUN_004cc060  scene_fade_out
 *   FUN_004cc080  scene_fade_in        FUN_004cc0a0  scene_wait           FUN_004cc0c0  scene_wait_fade
 *   FUN_00403100  scene_area_title     FUN_0041ec20  scene_place_at_door  FUN_0041f0e0  scene_spawn_at_door
 *   FUN_004cbf80  scene_place_rec      FUN_004cca70  room_bgm_restart
 *   FUN_004061d0  cast_cmd_clear       FUN_00402330  scene_cast_to        FUN_004d64b0  scene_cast_to_rec
 *   FUN_00402730  scene_cast_step      FUN_00405e80  scene_party_gather   FUN_0041a2a0  scene_yes_no
 *   FUN_00431d10  scene_device_spawn
 */
#ifndef OSS_SCENE_CMD_H
#define OSS_SCENE_CMD_H

#include <stdint.h>

#include "world_layout.h"

/* 0x49d6e0 (thiscall runner): a message box (+0x20 = 1).  `kind` 0..2 = spoken by `actor` (its facing picks
 * the portrait; none: nothing), `name` the box title (else the actor's party name when `own_name`),
 * `voice` != 0 / 32 the voice row of 0x6b6568 by (actor record, voice), `face` the portrait (config +0x1ac) */
void scene_msg(wl_event *ev, uint32_t kind, void *actor, const char *text, int32_t a5, int32_t a6, int32_t own_name,
               const char *name, int32_t voice, int32_t a10, uint16_t face);
/* 0x4a5ee0: scene_msg by the speaker's map record */
void scene_msg_rec(wl_event *ev, uint32_t kind, int32_t rec, const char *text, int32_t a5, int32_t a6,
                   int32_t own_name, const char *name, int32_t voice, int32_t a10, uint16_t face);
/* 0x401e60 (thiscall runner): `actor` turns to `dir` (cast command 2; +0x20 = 4) */
void scene_face(wl_event *ev, void *actor, int32_t dir);
/* 0x4a5e20: scene_face by map record */
void scene_face_rec(wl_event *ev, int32_t rec, int32_t dir);
/* 0x402d40 (thiscall runner): the party's weapon stance (Arche / 100165 / 100167 / 100168: party +0xd4 0 →
 * kind 3 when `draw`, 1 → kind 4 when not); 1 when one changes (+0x20 = 4) */
int scene_party_stance(wl_event *ev, int32_t draw, wl_room *room);
/* 0x406210 (thiscall runner): the leader turns to `target` (stepping back when `step` and nearer than
 * 3200), the target faces back when `face_back`; then the party gathers (0x405e80) */
int scene_talk_face(wl_event *ev, int32_t a2, void *target, int32_t face_back, int32_t step);
/* 0x41d190 (thiscall runner): the room flag `id` set to (v, v2); both 0 frees it */
void script_flag_set(wl_event *ev, int32_t id, int32_t v, int32_t v2);
/* 0x4a5f50 (thiscall runner): the camera command (+0x20 = 3: +0x5c the target, +0x50 / +0x54, +0x58 the speed) */
int32_t scene_camera(wl_event *ev, int32_t target, int32_t a3, int32_t a4, int32_t speed);
/* 0x401d40 (thiscall runner): the room loop's next scene event (+0x900 / +0x904 / +0x908) */
void scene_next_event(wl_event *ev, int32_t ev_id, int32_t arg, int32_t face);
/* 0x4a5f30 (thiscall runner): +0x20 = 6, +0x57c = n */
int32_t scene_pause(wl_event *ev, int32_t n);
/* 0x4a5f90 (thiscall runner): +0x2e4 = 1 */
void scene_mark(wl_event *ev);
/* 0x4cc060 / 0x4cc080 (thiscall runner): the fade (+0x30 = 1, +0x34 3 / 2, +0x3c the time, +0x38 0 / 1) */
int32_t scene_fade_out(wl_event *ev, int32_t t);
int32_t scene_fade_in(wl_event *ev, int32_t t);
/* 0x4cc0a0 / 0x4cc0c0 (thiscall runner): the wait (+0x20 = 2, +0x24 = 1, +0x28 1 / 2, +0x2c the time) */
int32_t scene_wait(wl_event *ev, int32_t t);
int32_t scene_wait_fade(wl_event *ev, int32_t t);
/* 0x403100 (thiscall runner): the area title for `dur` ticks (+0x20 = 9) */
void scene_area_title(wl_event *ev, uint16_t dur);
/* shared with the room arrival: the area's name (by the room record's map) into the title node */
void scene_title_show(wl_roomloop *rl, uint16_t dur);
/* 0x41ec20 (thiscall runner): `actor` stands at door `door` + (dx, dy); `facing` (99 = keep) */
void scene_place_at_door(wl_event *ev, void *actor, int32_t door, int32_t dx, int32_t dy, int32_t facing);
/* 0x4cbf80: scene_place_at_door by the actor's map record */
void scene_place_rec(wl_event *ev, int32_t rec, int32_t door, int32_t dx, int32_t dy, int32_t facing);
/* 0x4cca70 (thiscall room loop): the room BGM again (0x40d9b0's pick, unfixed) with fade 20 */
void room_bgm_restart(wl_roomloop *rl, int32_t a2, int32_t a3);
/* 0x41f0e0 (thiscall runner): the next free world character (room loop +0xcc) initialised at door `door`
 * (+ dx, dy) as `code`; 0 without one */
wl_char *scene_spawn_at_door(wl_event *ev, int32_t handle, int32_t code, int32_t door, int32_t dx, int32_t dy,
                             int32_t facing, int32_t a8, int32_t a9);

/* 0x4061d0 (thiscall char): its cast command cleared (type 0) */
void cast_cmd_clear(uint8_t *a);
/* 0x402330 (thiscall runner): `actor` walks (cast command 1) to the first live map actor of `code` (any when 0)
 * + (dx, dy) (its x, its part-0 bottom); +0x15b74 / +0x15b88 / +0x15b78 = f74 / f88 / f78, the cast record's
 * +0x10 = r10 (+0x20 = 4) */
void scene_cast_to(wl_event *ev, void *actor, int32_t code, int32_t dx, int32_t dy, int32_t f74, int32_t f88,
                   int32_t f78, int32_t r10);
/* 0x4d64b0: scene_cast_to by map record */
void scene_cast_to_rec(wl_event *ev, int32_t rec, int32_t code, int32_t dx, int32_t dy, int32_t f74, int32_t f88,
                       int32_t f78, int32_t r10);
/* 0x402730 (thiscall runner): `c` walks to its own spot + (dx, dy) (its feet) */
void scene_cast_step(wl_event *ev, void *c, int32_t dx, int32_t dy, int32_t f74, int32_t f88, int32_t f78);
/* 0x405e80 (thiscall runner): every active member walks to the leader's floor facing `dir`, at x (the leader's
 * unless `use_x`) + its follow offset (+0x14254 when +0x1424c; mirrored unless facing 3) or 4000 apart when
 * `spread`; the leader leads, the others `lead` (+0x20 = 4); returns 0 */
int scene_party_gather(wl_event *ev, int32_t lead, int32_t dir, int32_t spread, int32_t use_x, int32_t x);
/* 0x41a2a0 (thiscall runner): the Yes / No choice (+0x20 = 5): 14 yes, 15 no, 6 the scene ended */
int scene_yes_no(wl_event *ev);
/* 0x431d10 (thiscall runner): a device `code` / `handle` / `kind` in the first free actor slot at (dx, dy), or
 * relative to map actor `near` (its x, its bottom); NULL without a free slot / that actor */
wl_actor *scene_device_spawn(wl_event *ev, int32_t handle, uint32_t code, int32_t near, int32_t dx, int32_t dy,
                             int32_t kind);

#endif /* OSS_SCENE_CMD_H */
