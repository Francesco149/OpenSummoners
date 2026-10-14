/*
 * world_layout.h — RETAIL-LAYOUT types for the in-game world (the 2026-10-14 rhost
 * methodology: ported functions run on retail's own memory inside rhost, so the
 * structs they touch must have retail's byte layout).  Offsets are pinned with
 * _Static_assert on the 32-bit build; the 64-bit host build keeps the same field
 * NAMES (pointers grow, so offsets differ there — code is offset-agnostic), exactly
 * like input_mgr (input.h).  Add fields as ported functions need them; pad bytes
 * are `_pNNN` (NNN = the hex offset they start at).
 *
 *   wl_world   the world singleton *(0x8a9b50) (0x27b8 bytes, ctor 0x4017d0)
 *   wl_entity  the common ENTITY base every world band shares (smallest band
 *              object is 0x1f8 bytes; the 32 full characters at world+0x1160
 *              are 0x15bb0 bytes and extend it)
 *   wl_body    one 0x294-byte body/render-state record (entity+0x40 points at an
 *              array of entity+0x44 of them; the movers take one by part index)
 *   wl_list    the 0xc-byte entity list objects (world+0x2788 / +0x278c)
 */
#ifndef OSS_WORLD_LAYOUT_H
#define OSS_WORLD_LAYOUT_H

#include <stddef.h>
#include <stdint.h>

#include "collision.h"   /* coll_slope_fn */

struct wl_entity;

typedef struct wl_body {
    uint8_t  active;          /* +0x000  0 => part absent (the movers' gate)            */
    uint8_t  _p001[3];
    int32_t  x;               /* +0x004  world X (px*100)                               */
    int32_t  y;               /* +0x008  world Y                                        */
    int32_t  w;               /* +0x00c  box width                                      */
    int32_t  h;               /* +0x010  box height                                     */
    int32_t  margin;          /* +0x014  top inset: the collision top = y + margin      */
    uint8_t  _p018[0x7c - 0x18];
    uint8_t  nocollide;       /* +0x07c  != 0 => ignores / is ignored by entity checks  */
    uint8_t  _p07d[0x284 - 0x7d];
    struct wl_entity *attach; /* +0x284  the entity this body rides/belongs to          */
    uint8_t  _p288[0x294 - 0x288];
} wl_body;

typedef struct wl_entity {
    uint8_t  _p000[0x38];
    int32_t  no_entity_coll; /* +0x038  != 0 => the movers skip the entity scan        */
    uint8_t  _p03c[0x40 - 0x3c];
    wl_body *body;           /* +0x040  body array                                     */
    uint16_t nbody;          /* +0x044  parts in it                                    */
    uint8_t  _p046[0x104 - 0x46];
    int32_t  coll_mode;      /* +0x104  0 none, 1 solid to non-nocollide bodies,       */
                             /*         2 + the faction/state gate (0x4416c0)          */
    int32_t  coll_kind;      /* +0x108  1 side wall, 2 one-way platform, 3 solid block */
    uint8_t  _p10c[0x1dc - 0x10c];
    int32_t  faction;        /* +0x1dc  0x4416c0 operand                               */
    uint8_t  _p1e0[0x1f0 - 0x1e0];
    int32_t  coll_off;       /* +0x1f0  != 0 => no mode-2 collision                    */
    uint8_t  _p1f4[0x1f8 - 0x1f4];
} wl_entity;

typedef struct wl_list {
    uint32_t    _p0;
    wl_entity **items;       /* +0x4 */
    uint16_t    count;       /* +0x8 */
} wl_list;

typedef struct wl_world {
    uint8_t    _p0000[0x1048];
    uint8_t   *grid;         /* +0x1048  the 0x2cb044-byte runtime grid (map_grid.h)   */
    uint8_t    _p104c[0x2788 - 0x104c];
    wl_list   *list_2788;    /* +0x2788 */
    wl_list   *collide;      /* +0x278c  entities the movers test against              */
    uint8_t    _p2790[0x27b8 - 0x2790];
} wl_world;

#if UINTPTR_MAX == 0xFFFFFFFFu
_Static_assert(offsetof(wl_body, x)         == 0x004, "body x");
_Static_assert(offsetof(wl_body, margin)    == 0x014, "body margin");
_Static_assert(offsetof(wl_body, nocollide) == 0x07c, "body nocollide");
_Static_assert(offsetof(wl_body, attach)    == 0x284, "body attach");
_Static_assert(sizeof(wl_body)              == 0x294, "body is 0x294 bytes");
_Static_assert(offsetof(wl_entity, no_entity_coll) == 0x038, "entity +0x38");
_Static_assert(offsetof(wl_entity, body)     == 0x040, "entity body");
_Static_assert(offsetof(wl_entity, nbody)    == 0x044, "entity nbody");
_Static_assert(offsetof(wl_entity, coll_mode) == 0x104, "entity coll_mode");
_Static_assert(offsetof(wl_entity, coll_kind) == 0x108, "entity coll_kind");
_Static_assert(offsetof(wl_entity, faction)  == 0x1dc, "entity faction");
_Static_assert(offsetof(wl_entity, coll_off) == 0x1f0, "entity coll_off");
_Static_assert(sizeof(wl_entity)             == 0x1f8, "entity base is 0x1f8 bytes");
_Static_assert(offsetof(wl_list, count)      == 0x8,   "list count");
_Static_assert(offsetof(wl_world, grid)      == 0x1048, "world grid");
_Static_assert(offsetof(wl_world, collide)   == 0x278c, "world collide list");
_Static_assert(sizeof(wl_world)              == 0x27b8, "world is 0x27b8 bytes");
#endif

/* The port's *(0x8a9b50) + the slope-profile resolver the tile movers use (a sloped
 * region-B cell's +0x8 is a retail .rdata VA; the port reads it off the user's exe,
 * rhost reads the mapped image). */
extern wl_world     *g_world;
extern coll_slope_fn g_world_slope;
extern void         *g_world_slope_ctx;

#endif /* OSS_WORLD_LAYOUT_H */
