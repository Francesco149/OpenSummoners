/*
 * particle_init.{c,h} — the PARTICLE INIT FUN_00557550 (the per-code setup the spawner
 * 0x557370 runs on a fresh slot) and the generic ENTITY SETUP FUN_00426620 with the small
 * helpers it uses (retail layout; rhost-verified inside retail).
 *
 * The init clears the particle state (+0x250..+0x280; owner, priority), then per code
 * (100042..100137): the entity setup (kind 4, the code, the faction, the box at the
 * anchor, a tint table index), a track record (0x426d70 / 0x426db0), a clip, the tints
 * (sys tables +0x2758.. / +0x27a8..), launch velocities (0x453960), a lifetime, a parent
 * to ride (0x55cd50 / 0x476c70).  Sixteen codes inline the entity setup through the anchor
 * helper 0x41ee60 — identical effects, so they share entity_setup here.  The tail sets the
 * facing (unless 99) and the body angle (+0x34, mirrored when facing left), the orbit
 * direction vector (x87, 0x460e40), a looping channel (0x55cbc0) and the delayed start.
 *
 *   FUN_00557550  particle_init          FUN_00426620  entity_setup
 *   FUN_004265e0  entity_fx_reset        FUN_00426d70  entity_track_on
 *   FUN_00426ec0  body_clip_jitter       FUN_004272e0  parts_free
 *   FUN_00438a60  structure_init
 *   FUN_004385c0  entity_tint            FUN_00448c80  tint_fade_a
 *   FUN_00476710  tint_fade_b            FUN_00476c70  entity_ride_part
 *   FUN_00476d00  body_set_angle         FUN_0055cbc0  entity_snd_alloc
 *   FUN_0055cd50  entity_ride            FUN_0055ce50  ember_len
 *   FUN_00460e40  polar_div              FUN_0055cdb0  polar_milli
 */
#ifndef OSS_PARTICLE_INIT_H
#define OSS_PARTICLE_INIT_H

#include <stdint.h>

#include "world_layout.h"

/* code `code` at (x, y) (`anchor` 0x41ee60), `facing` (99 = keep), `on` != 0 starts it
 * inactive (a delayed start of `on` ticks), `a7` the track phase, `owner`, `a9` the angle
 * (degrees), `prio`, the spawner's faction. */
int32_t particle_init(wl_particle *p, int32_t code, int32_t x, int32_t y, int32_t facing, uint32_t delay, int32_t anchor,
                      wl_entity *owner, int32_t angle, int32_t prio, int32_t fac);
/* 0x438a60 (thiscall entity): a map STRUCTURE (world +0x2560; kind 5): its box + track by code
 * (60501..60528 / 60540 / 61101 / 62101; others stay as they are), then the handle `h` */
void structure_init(wl_entity *e, int32_t x, int32_t y, int32_t anchor, uint32_t code, int32_t trk_b, int32_t tint,
                    int32_t h);

void entity_setup(wl_entity *e, int32_t kind, int32_t code, int32_t fac, int32_t x, int32_t y, int32_t w, int32_t h,
                  int32_t facing, uint8_t on, int32_t dx, int32_t dy, int32_t anchor, int32_t tint, int16_t nfx);
void entity_fx_reset(wl_entity *e, int16_t n);
int32_t entity_track_on(wl_entity *e, uint16_t i, int16_t a, int16_t b);
void body_clip_jitter(wl_body *b);
void parts_free(void *t, int32_t del);
int32_t entity_tint(wl_entity *e, int32_t t);
int32_t tint_fade_a(int32_t k, int32_t n);
int32_t tint_fade_b(int32_t k, int32_t n);
void entity_ride_part(wl_entity *e, const wl_body *b, uint16_t part);
void body_set_angle(wl_body *b, int32_t a, int32_t mirror);
int entity_snd_alloc(wl_entity *e, uint16_t n);
void entity_ride(wl_entity *e, wl_entity *parent);
int32_t ember_len(int32_t a);
/* (cx + round(r sin a), cy - round(r cos a)), a = ang * deg / n  (x87, as retail) */
void polar_div(int32_t r, int32_t ang, int32_t n, int32_t cx, int32_t cy, int32_t *ox, int32_t *oy);
/* the same with a = ang * deg * 0.001 */
void polar_milli(int32_t cx, int32_t cy, int32_t r, int32_t ang, int32_t *ox, int32_t *oy);

#endif /* OSS_PARTICLE_INIT_H */
