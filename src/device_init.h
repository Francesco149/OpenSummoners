/*
 * device_init.{c,h} — the map DEVICE init 0x431e30 (retail layout; rhost-verified inside retail).
 *
 * A device is an actor (world +0x11e0) the room population (or a device that spawns a part,
 * 0x556ef0) sets up from a map object: its sound channels are reset to one (0x426490), the
 * per-device fields filled from the object's arrays (area +0x2b4..+0x2c0, the parameters
 * +0x2cc..+0x2d8, the goal +0x27c / +0x280, the link +0x298, the kind +0x28c, ...), +0x2e0 = the
 * row of the 0x693808 table keyed by the room record's +0x140; then by CODE (70100..70910, 71010,
 * 71031, 100138..100145; any other code is fatal: "Unknown Object Type Device.") its box (the
 * entity setup 0x426620: kind 2, tint 9, 4 effect records), its track (0x426d70 or the full
 * record 0x426db0), its collision, its switch-step records (0x438630: state, clip, sound), its
 * zone (0x438670), tints and flags.  Then the ride mode (the object's, else 3 for the solid ones),
 * the saved switch state (0x41e600), the handle (0x555f00), the lists (+0x2788 with a serial,
 * +0x2794 for the movers, +0x2798 for the material ones), the other parts copied from part 0
 * (0x438740) unless multipart, and the offset to the entity it follows.
 *
 *   FUN_00431e30  device_init          FUN_00426490  entity_snd_free
 *   FUN_00438630  device_step_add      FUN_00438610  device_tint
 *   FUN_00438670  device_zone          FUN_004386a0  device_link_state
 *   FUN_004386f0  device_night_glow    FUN_00438520  grid_mark_d
 *   FUN_00438740  device_parts_copy    FUN_00556ef0  device_spawn
 *   FUN_004385c0  entity_tint_fx       FUN_00427330  wl_list_push
 */
#ifndef OSS_DEVICE_INIT_H
#define OSS_DEVICE_INIT_H

#include <stdint.h>

#include "world_layout.h"

/* 0x431e30 (thiscall actor, 25 args) */
void device_init(wl_actor *a, int32_t x, int32_t y, int32_t anchor, uint32_t code, int32_t kind6, int32_t trk,
                 int32_t handle, int32_t zone, int32_t state, int32_t kind, int32_t facing, int32_t x0, int32_t x1,
                 int32_t y0, int32_t y1, int32_t p0, int32_t p1, int32_t p2, int32_t p3, int32_t gx, int32_t gy,
                 int32_t link, int32_t ride, int32_t map, wl_entity *follow);
/* 0x426490 (thiscall entity): stop and drop its looping-sound channels */
void entity_snd_free(wl_entity *e);
/* 0x438630 (thiscall actor): append a switch-step record (up to 8) */
void device_step_add(wl_actor *a, int32_t state, const void *clip, uint16_t sfx);
/* 0x438610 (thiscall actor): both tint words */
void device_tint(wl_actor *a, int32_t t);
/* 0x438670 (thiscall actor): an event zone of `kind` with id `id` (none for 0) */
void device_zone(wl_actor *a, int32_t kind, int32_t id);
/* 0x4386a0 (thiscall actor): its state from the linked actor's (+0x2cc == 1 follows, else inverts) */
int32_t device_link_state(const wl_actor *a, int32_t st);
/* 0x4386f0 (thiscall actor): the night glow particle at (dx, dy) when the time of day is 2..3 */
void device_night_glow(wl_actor *a, int32_t dx, int32_t dy);
/* 0x438520 (thiscall grid): region D = v over the cells of the box */
void grid_mark_d(uint8_t *grid, int32_t x, int32_t y, int32_t w, int32_t h, uint16_t v);
/* 0x438740 (thiscall actor): parts 1.. copy part 0 (and the rects into their frame records) */
void device_parts_copy(wl_actor *a);
/* 0x556ef0 (stdcall): a device `code` in the first free actor at the anchor-relative point of body
 * b (anchor 1 centre, 2 left-bottom, 3 centre-bottom, 4 centre-lower edge, else the point) */
wl_actor *device_spawn(wl_entity *owner, uint32_t code, const wl_body *b, int32_t dx, int32_t dy, int32_t facing,
                       int32_t anchor, int32_t none, int32_t anchor2);
/* 0x427330 (thiscall list): append while there is room; 1 when added */
int wl_list_push(wl_list *l, wl_entity *e);

#endif /* OSS_DEVICE_INIT_H */
