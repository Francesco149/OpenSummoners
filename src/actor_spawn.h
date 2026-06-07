/*
 * actor_spawn.{c,h} — the town CHARACTER-band spawn: turn the map's CHARACTER
 * object-placement layers into the renderable {actor, actor_render_state} pairs
 * the ckpt-77 actor_render path (actor_render_static / FUN_0044d160) consumes.
 *
 * Engine correspondence (proven: docs/proofs/map-object-layer-format.md):
 *   0x586010:698 -> 0x58d460 (the room object-population pass) walks the
 *   map's `count` object-placement layers; each 0x3c-byte layer header IS one
 *   object's placement record (+0x04 x, +0x08 y, +0x10 type code).  Objects with
 *   a type code in 70000..79999 are CHARACTER objects, dispatched into the
 *   +0x11e0 band by 0x431e30 (__thiscall, ECX = the free slot), which sets
 *   actor+0x1d4 = type, +0x1d0 = 1 (active), +0xfc = 9 (layer), +0xe8 = 0 (dir),
 *   stores world (x*100, y*100), and ZEROES the +0x48 per-direction sprite table.
 *
 * THE KEY FINDING (ckpt 79, live capture at the town hold — see the census in
 * findings/in-game-intro.md "The town actor RENDER CENSUS"):  the +0x48 sprite
 * table is NOT filled by 0x431e30 — it is filled LAZILY (the state machine
 * 0x40afe0/0x41e600 reads a def table keyed by the type + state).  And
 * of DATA 1022's 32 CHARACTER objects, only THREE codes actually draw — the rest
 * are invisible collision / trigger / spawn volumes (all-zero sprite table, the
 * renderer self-skips them via FUN_0044d160's `bank==0 => return 0`).  The three
 * visible codes all use sprite bank 0x16c (the town-OBJECTS sheet, res 0x403) and
 * are static PROPS, NOT people-NPCs (USER-confirmed: a fountain + a barrel):
 *     0x1129e -> bank 0x16c frame 1  layer 9    (x3)
 *     0x1129f -> bank 0x16c frame 2  layer 9    (x1)
 *     0x112e5 -> bank 0x16c frame 36 layer 10   (x1, the fountain)
 * (The town's only person, the animated protagonist code 0x1872d / bank 0x175, is
 * OUTSIDE the 70000 CHARACTER range — a SEPARATE spawn path — and needs the
 * 0x491ae0 0x1872d multi-part animated arm; both are deferred, not produced here.
 * A static people-NPC, if a scene had one, would ride this same path with its own
 * (bank,frame); the module name stays "actor" — the engine's band is CHARACTER.)
 *
 * PORT-DEBT(actor-sprite-table): the code->(bank,frame_base,layer) map below is
 * captured ground truth standing in for the lazy def-table fill.  It is
 * room-specific (DATA 1022).  Retire it by RE'ing + extracting the def table the
 * 0x40afe0/0x41e600 state-set reads (keyed by actor type+state), so any room's
 * actors get their sprites data-drivenly.  Until then the spawn faithfully
 * reproduces the END STATE (the 27 invisible volumes get bank 0, exactly as the
 * lazy fill leaves them) — only the THREE visible codes need the stand-in.
 *
 * Win32-free + pure: the spawn reads only the parsed map_data and fills logical
 * structs (host-tested against the live census).
 */
#ifndef OSS_ACTOR_SPAWN_H
#define OSS_ACTOR_SPAWN_H

#include <stdint.h>

#include "actor_render.h"   /* actor, actor_render_state */
#include "map_data.h"       /* map_data, map_layer       */

/* The +0x11e0 CHARACTER band is a pre-allocated 0x80-slot pool (0x58cf60
 * x128 at 0x586010:476-506).  The spawn activates a subset. */
#define ACTOR_BAND_SLOTS 128

/* CHARACTER type-code range (0x58d460's dispatch band; 70000..79999). */
#define ACTOR_CODE_CHARACTER_LO 70000u
#define ACTOR_CODE_CHARACTER_HI 79999u

/*
 * A spawned band: parallel {actor, render-state} arrays the render walk drives
 * (actor_render_static(&actors[i], &states[i], ...)).  The two are separate
 * objects in retail (the render-state is *(actor+0x40)); modelled as parallel
 * arrays here so both stay the LOGICAL structs actor_render.h defines.
 */
typedef struct actor_spawn_pool {
    int                count;                         /* active slots [0,count)  */
    actor              actors[ACTOR_BAND_SLOTS];
    actor_render_state states[ACTOR_BAND_SLOTS];
} actor_spawn_pool;

/*
 * Populate `pool` with the CHARACTER objects of the parsed map `md`:  for each
 * object-placement layer whose type code is in 70000..79999, activate one slot
 * (world pos = layer (x,y) * 100; dir 0; layer 9 default; static, clip NULL),
 * filling its dir-0 sprite row from the visible-code stand-in map (a bank-0 row
 * — i.e. an invisible volume — for every other code, matching retail's end
 * state).  Zeroes `pool` first.
 *
 * Returns the number of CHARACTER actors spawned (DATA 1022 -> 32), or -1 on a
 * NULL arg or if the map holds more than ACTOR_BAND_SLOTS character objects.
 */
int actor_spawn_from_map(actor_spawn_pool *pool, const map_data *md);

/*
 * The PORT-DEBT visible-code sprite map (exposed for the host test / a future
 * def-table cross-check).  Returns 1 and fills bank / frame_base / layer for a
 * code that draws, or 0 for an invisible code (caller leaves the row zeroed).
 */
int actor_spawn_sprite_for_code(uint32_t code, uint16_t *bank,
                                int16_t *frame_base, uint32_t *layer);

#endif /* OSS_ACTOR_SPAWN_H */
