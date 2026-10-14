/*
 * entity_fx.{c,h} — an entity's EFFECT RECORDS and LOOPING-SOUND CHANNELS (retail
 * layout; rhost-verified inside retail).
 *
 * Every world entity carries 8 effect records at +0x10c (wl_fx: an id 100146..100158, a
 * duration, a tick counter, a y; count +0x1ac).  The tick runs them (0x54d090): water
 * splashes with a pooled one-shot splash sound (100146..100149, added by the water
 * entry / exit check 0x46d7f0 or the character apply pass), dust / sparks / drips
 * (100150..100158) — particles through 0x557370.  An entity may also own an array of
 * looping-sound channels at +0x1b0 (wl_sndch, count +0x1b4): each tick the channel's
 * volume and pan follow the entity's first body relative to the listener (0x451200);
 * a channel its owner stopped refreshing (`live` == 0) fades -100 per tick to -5000,
 * then is released (0x4537d0); out of earshot it is released at once.
 *
 *   FUN_00448cb0  entity_fx_add          FUN_0044bbc0  entity_fx_remove
 *   FUN_0054d090  entity_fx_tick         FUN_0046d7f0  entity_splash
 *   FUN_004537d0  entity_snd_release     FUN_0046d180  entity_snd_update_all
 *   FUN_0046d4a0  entity_snd_update      FUN_00451200  view_listener
 */
#ifndef OSS_ENTITY_FX_H
#define OSS_ENTITY_FX_H

#include <stdint.h>

#include "camera_follow.h"
#include "world_layout.h"

/* Add effect record `id` (dur ticks, 0 = until removed): `replace` first drops every
 * record with that id, `unique` refuses when one is present, a7 != 0 refuses always.
 * Returns the record, or NULL (no free slot / refused). */
wl_fx *entity_fx_add(wl_entity *e, int32_t id, int32_t replace, int16_t dur, int32_t f0c, int32_t y,
                     int32_t a7, int32_t unique);
void entity_fx_remove(wl_entity *e, int32_t id);
/* One tick of every effect record; `b` = the body the effects attach to, `part` its index. */
void entity_fx_tick(wl_entity *e, wl_body *b, uint16_t part);
/* The water entry / exit splash: `was_slow` / `was_c` = the body's +0x288 / +0x28c flags
 * before this tick's move. */
void entity_splash(wl_entity *e, wl_body *b, int32_t was_slow, int32_t was_c);

/* Release channel i (a3 != 0: no-op; `stop` also stops the buffer). */
void entity_snd_release(wl_entity *e, uint16_t i, intptr_t a3, int32_t stop);
void entity_snd_update_all(wl_entity *e);
void entity_snd_update(wl_entity *e, uint16_t i);
void view_listener(const camera_view *v, int32_t *x, int32_t *y);

#endif /* OSS_ENTITY_FX_H */
