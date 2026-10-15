/*
 * char_status.{c,h} — a character's per-tick STATUS pass 0x487dc0 and its fade 0x4878b0 (the tail of the
 * apply pass 0x485fc0), with the leaves they need (retail layout; rhost-verified inside retail).
 *
 * The status timers are the party's u16 +0x448[18] (+0x490[18] their values).  0x487dc0, by status: 4
 * (doom) the fall 0x5b4d00, 3 the stun mark particle 100113, 1 weakened (strength 12; cured at once in scene
 * kind 2), 6 poison (bubbles 100107, a 5 % hit every 256 ticks — 512 for the party), the damaging floor (a 2 %
 * hit every 256 ticks on grid C 3), 2 paralysed (pose 118), 8 (pose 119 + the tint), 9 a carried partner
 * (pose 121), 7 defense down (+0xc7dc / +0xc7e0 lowered, 100108), 5 asleep (pose 117), 10 fallen (pose 109
 * → 108), 15 hasted (+20 to the four speeds, the after-images 100131), 17 the four orbit shots 100015, 16
 * the two shields 100014 (worn out after 2 hits), 13 attack up (+0xc7d8, 100109), 12 the guard sound and
 * radius, 14 / 11 the auras 100023 / 100024; any pose status first drops the character's ten shots.  Then
 * (unless intangible) each status with a +0x5c row flag is cured when no foe is within 48000 x 20000, and
 * each with a +0x6c flag counts down (0x48bb30).  0x4878b0 runs the +0x158a8 fade: 1 out (sound 0x4ec,
 * the body sinks to nothing), 2 in (sound 0x4f0); while it shows, the sprite's rows shed sparkles 100049 /
 * 100098 from the cut line.
 *
 *   FUN_00487dc0  char_status_tick     FUN_004878b0  char_fade_tick       FUN_0048bb30  char_status_decay
 *   FUN_004891c0  char_hit_queue       FUN_0044e6c0  world_log_line       FUN_0044d020  entity_part_descs
 *   FUN_0044d160  entity_sprite_desc   FUN_0055cfc0  shot_spawn
 */
#ifndef OSS_CHAR_STATUS_H
#define OSS_CHAR_STATUS_H

#include <stdint.h>

#include "world_layout.h"

/* one sprite descriptor (20 B: the renderer's draw request) */
typedef struct es_desc {
    int32_t  x, y;           /* +0x0 / +0x4 */
    uint16_t slot, frame;    /* +0x8 / +0xa */
    int32_t  f0c;            /* +0xc  (the part record's first word) */
    int32_t  alpha;          /* +0x10 */
} es_desc;

/* 0x487dc0 (thiscall char) */
void char_status_tick(wl_char *c, wl_body *b);
/* 0x4878b0 (thiscall char) */
void char_fade_tick(wl_char *c);
/* 0x48bb30 (thiscall char): status `st`'s timer less `n` (`now`: cleared); at 0 the cure line, into the
 * message log when `log`, and to `out` */
void char_status_decay(wl_char *c, uint16_t st, uint16_t n, int32_t now, uint32_t log, char *out);
/* 0x4891c0 (thiscall char): the pending hit (+0x15aa8..+0x15ae8, +0x26e = 1) */
int32_t char_hit_queue(wl_char *c, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7, int32_t a8,
                       int32_t a9, int32_t a10, int32_t a11, int32_t a12, int32_t a13, int32_t a14, int32_t a15,
                       int32_t a16, int32_t a17);
/* 0x44e6c0 (thiscall world): `text` into the first free of the 8 message-log slots from +0x1024 */
void world_log_line(wl_world *w, const char *text);
/* 0x44d020 (thiscall entity): the multi-part frame's descriptors (+0x8 table, by the clip frame / facing) */
int entity_part_descs(const wl_entity *e, const wl_body *b, es_desc *out, uint16_t *n);
/* 0x44d160 (thiscall entity): the body's sprite descriptor (the row +0xe8, the clip frame, facing / angle) */
int entity_sprite_desc(const wl_entity *e, const wl_body *b, es_desc *out);
/* 0x55cfc0 (thiscall): a shot from world+0x1060 (the room loop's cursor +0xd2) at `b` + (dx, dy) by `anchor`
 * (1 centre, 2 left / top, 3 centre / top, 4 centre / bottom, else absolute), initialised by 0x55d140 */
void *shot_spawn(void *self, int32_t a2, int32_t a3, int32_t a4, wl_body *b, int32_t dx, int32_t dy, int32_t anchor,
                 int32_t a9, int16_t a10, int32_t a11, int32_t a12, int32_t a13, int32_t a14, int32_t a15, int32_t a16,
                 int32_t a17, int32_t a18, int32_t a19, int32_t a20);

#endif /* OSS_CHAR_STATUS_H */
