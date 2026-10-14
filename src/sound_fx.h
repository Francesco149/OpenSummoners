/*
 * sound_fx.{c,h} — the engine's SOUND-EFFECT layer on the retail layout: the voice
 * pools (lazy wave load, round-robin take), placing a sound relative to the listener,
 * the one-shot players, a character's voice lines, and the start of an entity's
 * looping channel.  Faithful; rhost-verified inside retail.  The DirectSound buffer
 * calls (volume / pan / frequency / play / stop 0x5bb870..0x5bb8e0, create / duplicate
 * 0x5bb250 / 0x5bb2f0) stay engine_calls slots: the platform boundary.
 *
 * Placement (every positional sound): the listener (0x451200) against the entity's
 * first body — dx = 0 inside its span, else the nearer edge; dy = body y + entity +0x24
 * - listener y.  volume = base + 3000 * (effects per mille - 1000) / 1000
 *   + falloff * max(0, |dx| - 8000) / 120000 + falloff * max(0, |dy| - 16000) / 48000,
 * clamped to [-10000, 0]; pan = 10000 * dx / 128000.  falloff 0 means -5000, 1 means
 * unplaced (no attenuation, centre pan), -16000 also culls beyond 19200 x 11200; beyond
 * 128000 x 64000 the sound is culled.
 *
 *   FUN_00409e90  vpool_load          FUN_0040b710  vpool_take
 *   FUN_0040bb90  sfx_place           FUN_00411390  sfx_play
 *   FUN_00407ba0  sfx_play_at         FUN_0040a010  voice_place
 *   FUN_004099a0  voice_play          FUN_00450ef0  entity_snd_start
 *   FUN_00445fd0  voice_cue           FUN_004463d0  vline_stop
 *   FUN_00446400  voiceset_tick       FUN_004464f0  voiceset_sounding
 *   FUN_00446580  voiceset_cut        FUN_00489280  sfx_play_jit
 *   FUN_00476880  entity_snd_loop     FUN_005531b0  ambient_sound_timer
 *   FUN_00467380  char_idle_voice
 */
#ifndef OSS_SOUND_FX_H
#define OSS_SOUND_FX_H

#include <stdint.h>

#include "world_layout.h"

/* sound pool `slot`: *(sys + 0x364 + 4 slot) */
wl_vpool *sound_pool(uint32_t slot);

/* Load a pool's buffers (one created, the rest duplicated from it).  1 = loaded; 0 = no
 * device, already loaded, or failed ("Failed Wave Load" to the error log). */
int vpool_load(wl_vpool *p);
/* Take the next free voice round-robin (loading on first use), marking it `owner`
 * (0 = a one-shot nobody holds).  NULL when none. */
wl_voice *vpool_take(wl_vpool *p, int32_t owner);

/* Volume / pan of a sound at entity `e`'s body `b` (see above).  `keep` != 0 skips the
 * 128000 x 64000 cull.  0 = culled (outputs untouched). */
int sfx_place(const wl_entity *e, int32_t base, int32_t falloff, int32_t keep, const wl_body *b,
              int32_t *out_vol, int32_t *out_pan);
/* A non-positional one-shot from pool `id`: volume = vol + the sfx volume (unclamped). */
int sfx_play(void *sys, uint16_t id, int32_t vol, int32_t freq);
/* A positional one-shot from pool `id` at `e` (a2 != 0: nothing).  The voice is taken
 * before the cull.  0 = culled, else 1. */
int sfx_play_at(wl_entity *e, int32_t a2, uint16_t id, int32_t vol, int32_t freq, int32_t falloff);
/* Where a character's voice line sounds: voice mode -> the voice volume, falloff -2000,
 * freq 0; else the sfx volume, falloff -5000, freq 500 - rand 0..999.  0 = out of
 * earshot (*vol = -10000, *pan = 0). */
int voice_place(const wl_char *c, int32_t *vol, int32_t *pan, int32_t *freq);
/* Play a random not-yet-used line of group `grp` (cooldown, chance, priority vs the line
 * already playing: `force` lets an equal priority cut it).  1 = started. */
int voice_play(wl_voiceset *vs, uint16_t grp, int32_t force, int32_t vol, int32_t pan, int32_t freq, int32_t fixed);
/* A character's voice CUE (group `grp`, a2 != 0 / grp 0: nothing; voice mode 1 needs the
 * voices option): `delay` > 0 queues it (+0xc76c..+0xc778; a lower priority than the queued
 * one is refused), else it plays now — placed at the character (or at the voice volume
 * when `fixed`).  1 = queued / started. */
int voice_cue(wl_char *c, int32_t a2, uint16_t grp, int32_t force, int32_t delay, int32_t fixed);
/* the voice set's pieces: a line's stop (`stop` also stops the buffer), the clock (dt ms:
 * cooldowns, fades, line ends), the first sounding (not fading) line, cut / fade it by mode */
void vline_stop(wl_vline *l, int32_t stop);
void voiceset_tick(wl_voiceset *vs, int32_t dt);
int voiceset_sounding(const wl_voiceset *vs, wl_vgroup **g, wl_vline **l);
void voiceset_cut(wl_voiceset *vs);
/* Start looping channel i from pool `id` (a3 != 0: nothing; culled by placement unless
 * falloff 1): releases what it held (`stop` also stops that buffer), reserves a voice,
 * plays it at -10000 (the channel tick fades it in). */
int entity_snd_start(wl_entity *e, uint16_t i, int32_t a3, uint16_t id, int32_t vol, int32_t freq,
                     int32_t falloff, int32_t stop);
/* A placed one-shot (falloff -5000) from pool `id` with volume jitter: v = (kind 2 ?
 * vol_a : vol_b), volume 750 v / 1000 + v * rand 0..499 / 1000; freq + jit * rand / 0x8000.
 * The voice is taken before the cull. */
int sfx_play_jit(wl_entity *e, uint16_t id, int32_t kind, int32_t vol_a, int32_t vol_b, int32_t freq, int32_t jit);
/* Keep looping channel i going: when its period counter (+0x20) is 0 and the sound is in
 * earshot, (re)start it on a reserved voice (at -10000; the channel tick fades it in);
 * every call keeps it live and steps the counter (wraps at `period`). */
void entity_snd_loop(wl_entity *e, uint16_t i, int32_t a3, uint16_t id, int32_t vol, int32_t freq, int32_t falloff,
                     int32_t stop, int32_t period);
/* An actor's AMBIENT sound timer (body +0x58 armed / +0x5c countdown): only when the
 * world allows it (no scene with `scene_off`, no world+0x27a8 lock) and the time slot
 * 0x8a93fc (1 / 2..4 / 5 / else) is enabled; every t0 + tv * rand / 0x8000 ticks it picks
 * one of up to 4 sounds and (re)starts channel 0 on it; between, channel 0 stays live. */
int ambient_sound_timer(wl_entity *e, wl_body *b, int32_t probe, uint16_t t0, uint16_t tv, uint16_t s0, uint16_t s1,
                        uint16_t s2, uint16_t s3, uint16_t ns, int32_t on_def, int32_t on_1, int32_t on_2_4, int32_t vol,
                        int32_t f0, int32_t fv, int32_t scene_off);
/* A character's IDLE voice (+0x208 on, u16 countdown +0x20c): at 1 it cues group `grp`
 * (forced, placed at the character); at 0 it reloads with t0 + (rand 0..999) % tr. */
void char_idle_voice(wl_char *c, int32_t probe, uint16_t grp, int32_t t0, uint16_t tr);
/* The one-shot idiom many callers inline: a voice of pool `p` (owner 0), placed at `e`'s
 * first body (`base` volume, `falloff`), played at `freq`.  1 = played. */
int sfx_oneshot(wl_entity *e, wl_vpool *p, int32_t base, int32_t falloff, int32_t freq);

#endif /* OSS_SOUND_FX_H */
