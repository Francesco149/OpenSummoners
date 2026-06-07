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
 * OUTSIDE the 70000 CHARACTER range — a SEPARATE cutscene spawn path, now RE'd +
 * produced by actor_spawn_protagonist below, drawn by the 0x491ae0 0x1872d
 * multi-part arm actor_render_protagonist.  A static people-NPC, if a scene had
 * one, would ride actor_spawn_from_map with its own (bank,frame); the module name
 * stays "actor" — the engine's band is CHARACTER.)
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

/* STRUCTURE type-code range (0x58d460's dispatch band; 60000..69999 -> the
 * +0x2560 pool, activator 0x438a60, rendered by the single-cel 0x493230 —
 * which produces a BIT-IDENTICAL static blit to actor_render_static, so the
 * STRUCTURE band reuses that renderer).  The town's scenery: the foreground TREE
 * (0xec55), bg decorations (0xec6a), fg hedges (0xec60).  engine-quirk #84. */
#define ACTOR_CODE_STRUCTURE_LO 60000u
#define ACTOR_CODE_STRUCTURE_HI 69999u

/* EFFECT type-code range (0x58d460's dispatch band; 50000..59999 -> the +0x1160
 * pool, activator 0x41f200, rendered by the multi-part 0x493ba0).  The town's
 * townsfolk — the standing villagers in the square.  For a plain (non-spell,
 * non-shadowed) townsperson the 0x493ba0 static arm REDUCES to the same
 * describe (FUN_0044d160) + emit (FUN_00492670 = draw_pool_emit_actor) the
 * 0x491ae0 default arm runs (empirically: each townsperson emits exactly ONE
 * mode-0 keyed cel at the hold, no shadow/0x4917b0 split, no color-remap), so
 * the EFFECT band REUSES actor_render_static — exactly as STRUCTURE does.
 * engine-quirk #84. */
#define ACTOR_CODE_EFFECT_LO 50000u
#define ACTOR_CODE_EFFECT_HI 59999u

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

/*
 * Populate `pool` with the STRUCTURE objects of the parsed map `md`:  for each
 * object-placement layer whose type code is in 60000..69999, activate one slot.
 * Unlike CHARACTER, STRUCTURE is FULLY MAP-DRIVEN (engine-quirk #84, RE'd from
 * the activator 0x438a60 + the dispatcher 0x58d460):
 *   - world pos    = layer record (x@+0x04, y@+0x08) * 100
 *   - frame_base   = layer record's VARIANT (u16 @ +0x18) — verified cel-for-cel
 *                    live-vs-map (tree {0,1}, hedge {0,1,4,5}, deco {16,18,..,35})
 *   - draw layer   = record's foreground flag (+0x30): 1 => 15 (in front of the
 *                    cast), else 8 (behind)
 *   - sprite bank  = a per-code constant in 0x438a60's switch (the def table,
 *                    actor_spawn_struct_bank_for_code) — NOT captured, RE'd
 *   - static (clip NULL, dir 0)
 * Codes whose bank isn't in the def table are skipped (retail's switch `default`
 * draws nothing).  Zeroes `pool` first.  Returns the count spawned (DATA 1022 ->
 * 39: 0xec55 x2 + 0xec60 x8 + 0xec6a x29), or -1 on a NULL arg / pool overflow.
 */
int actor_spawn_struct_from_map(actor_spawn_pool *pool, const map_data *md);

/*
 * The STRUCTURE code -> sprite bank map (0x438a60's per-case fill: 0xec55->0x15f,
 * 0xec60->0x164, 0xec6a->0x16c, ...).  Returns 1 + *bank for a known structure
 * code, else 0.  RE'd from the activator (not a capture), so it is not PORT-DEBT.
 */
int actor_spawn_struct_bank_for_code(uint32_t code, uint16_t *bank);

/*
 * Populate `pool` with the EFFECT townsfolk of the parsed map `md`:  for each
 * object-placement layer whose type code is in 50000..59999 AND is one of the
 * standing-townsperson codes in the def table below, activate one slot.  Like
 * STRUCTURE the placement is FULLY MAP-DRIVEN, but the EFFECT band offsets the
 * world position by the render dst anchor (RE'd from the census + the map:
 * 0x41f200 stores world = (map (x,y) - dst) * 100, so the +0x40/+0x44 render-dst
 * shifts the SPRITE for centering while the +0x30..-px world offset keeps the
 * logical position the map point — the two cancel at the screen: screen = map -
 * cam).  Per townsperson:
 *   - world pos      = (map (x,y) @ +0x04/+0x08  -  dst) * 100
 *   - render dst     = rs +0x40/+0x44 = the per-code (dstx, dsty) anchor
 *   - sprite bank    = the per-code def-table value (PORT-DEBT, see below)
 *   - draw layer     = the per-code def-table value (12/13)
 *   - dir 0; clip NULL, frame_base 0 — FROZEN on the idle clip's frame 0 (the
 *     spawn end-state, pre-update).  The idle breathing clip (0x6290e0, 20
 *     frames, dur 14) + the per-actor anim PHASE are the next chip (Phase 1b);
 *     the phase is staggered run-deterministically but is NOT a map-record field
 *     (likely an RNG draw at spawn -> Phase 2), so it is deferred.
 *
 * Excludes the 4 wandering 0xe29a (RNG motion -> Phase 2) and the non-map
 * party/script townsfolk 0xc35a/0xc3dc/0xc3f0 (spawned like the wagon, outside
 * the map) — none are in the def table, so they are skipped.  Zeroes `pool`
 * first.  Returns the count spawned (DATA 1022 -> 11), or -1 on a NULL arg /
 * pool overflow.
 */
int actor_spawn_effect_from_map(actor_spawn_pool *pool, const map_data *md);

/*
 * PORT-DEBT(effect-sprite-table): the EFFECT code -> (bank, dst, layer) map,
 * CAPTURED from the retail town-hold census (live +0x48 / render-state read via
 * the 0x493ba0 field spec, flips 1450/1500/1600), standing in for 0x41f200's
 * per-type switch (a 27 KB function — captured like the CHARACTER band's
 * actor-sprite-table rather than RE'd case-by-case).  Returns 1 + the fields for
 * a known standing-townsperson code, else 0 (caller skips the object).  Retire
 * by RE'ing 0x41f200's town cases (bank/dst/layer install + the anim-phase
 * source).  The MAP supplies the position (x,y); only the appearance anchor is
 * captured.
 *
 * facing (rs +0x2c, 1 normal / 3 mirrored) and flip (the mirror frame offset =
 * DAT_008a8440[bank] first short = frames-per-direction) are out-params for the
 * facing==3 mirror; pass NULL to ignore.  facing is a deterministic MAP field
 * (dispatcher 0x58d460:96 from puVar1[4]), NOT RNG. */
int actor_spawn_effect_def_for_code(uint32_t code, uint16_t *bank,
                                    int16_t *dstx, int16_t *dsty, uint32_t *layer,
                                    int16_t *facing, int16_t *flip);

/* Fill a bank-indexed mirror/flip table (port stand-in for retail's global
 * DAT_008a8440) from the town EFFECT defs.  table[bank] = frames-per-direction;
 * FUN_0044d160 adds it to the frame on the facing==3 arm.  Only the town villager
 * banks are written (the only mirrored actors in the scene); other banks stay 0.
 * `n` is the table length (banks >= n are skipped).  Returns entries written. */
int actor_spawn_effect_fill_flip_table(int16_t *table, size_t n);

/* Append the town-intro cutscene party cast (the script-spawned characters
 * standing in front of the wagon: 0xc35a/0xc3dc/0xc3f0, spawned by 0x4d7d80
 * via the anchor-relative 0x41f0e0) to an already-filled EFFECT pool, AFTER
 * actor_spawn_effect_from_map.  Captured render-states (PORT-DEBT cutscene-cast);
 * rendered by the same actor_render_static path as the townsfolk (layer 13).
 * Also writes each cast bank's facing==3 mirror/flip value into `flip_table`
 * (the DAT_008a8440 stand-in; pass the same one actor_spawn_effect_fill_flip_table
 * filled, or NULL to skip).  Returns the number spawned (-1 on NULL pool). */
int actor_spawn_cutscene_cast(actor_spawn_pool *pool, int16_t *flip_table, size_t flip_n);

/*
 * The animated PROTAGONIST (code 0x1872d) — the town's one person.  It is NOT a
 * map CHARACTER object (its code is outside 70000..79999); it is spawned by the
 * TOWN INTRO CUTSCENE script 0x4d7d80 (case room-210110 / area 0xd2, gated
 * on event flags 0x5f76805 / 0x606aa4f) via the by-code main-band spawn helper
 *   0x431d10(0, 0x1872d, anchor=0x65, x=0x3200, 0, 0)
 * which lands in 0x431e30's case-0x1872d arm.  That arm installs, on the freshly
 * activated +0x11e0 slot:  sprite-table row 0 = {bank 0x175, frame_base 0} (via
 * 0x426db0(0, 0x175, 0, 1, 0, 0, 0)), render-state clip = &DAT_00671c48,
 * layer 9 (actor+0xfc), facing (+0x2c) = 99 (param_11), dir 0.  Full writeup:
 * findings/in-game-intro.md "The protagonist SPAWN".
 */
#define ACTOR_CODE_PROTAGONIST  0x1872du
#define ACTOR_PROT_SPRITE_BANK  0x175u   /* 0x426db0(0, 0x175, 0, ...)    */
#define ACTOR_PROT_FACING       99       /* render-state +0x2c (param_11 = 99) */

/*
 * Activate one band slot for the protagonist at world (world_x, world_y) and
 * return its slot index (or -1 if `pool` is NULL / full).  Fills the RE'd spawn
 * end-state above so actor_render_protagonist (the 0x491ae0 case-0x1872d arm)
 * draws the 3-cel composite.
 *
 * The render-state clip (&DAT_00671c48) is reconstructed from the RE'd values
 * (base_sprite 2, 4 frames, looping — the wagon's HORSES, sprite frames 2..5),
 * so the body cel draws the horses (frame 2), NOT a duplicate wagon half.  The
 * three composited cels are: wagon-body-left (frame 0) | wagon-body (frame 1) |
 * the horses (the animated body, sprite 2).
 *
 * The looping clip is driven once per sim-tick by actor_pool_update (the
 * 0x46cd70/0x54f980 stepper), so the horses TROT (body cel cycles sprite 2..5).
 * PORT-DEBT(actor-protagonist-clip) narrows to: the RNG-driven behaviour (idle
 * waits / wander, deferred ckpt 73) and the cutscene's anchor-relative roll-in
 * (the spawn pos is still the settled census const, not 0x431d10's anchor 0x65
 * arrival path).  Both are follow-ups.
 */
int actor_spawn_protagonist(actor_spawn_pool *pool, int32_t world_x, int32_t world_y);

/*
 * The per-SIM-TICK actor UPDATE walk — the 0x46cd70:123-169 main-band slice
 * (DAT_008a9b50+0x11e0, 0x80 slots).  Retail's per-tick driver 0x46cd70 calls
 * 0x54f980 for each active band actor; 0x54f980 runs (1) the deterministic
 * frame-stepper then (2) the RNG-driven behaviour.  This ports HALF (1): advance
 * every active actor whose render-state carries a clip by one tick
 * (actor_anim_advance).  In the opening town only the protagonist (0x1872d) has
 * a clip, so this trots its horses; the 32 static actors (clip NULL) no-op.
 * Behaviour half (2) — idle waits / wander — stays deferred (ckpt 73).
 *
 * Call once per sim tick (every 2nd Flip), on the SAME cadence as the camera
 * easer (retail 0x439690 runs 0x46cd70 at :1108, then the easer 0x43d1d0 at
 * :1123).  Returns the count of actors advanced (clip != NULL), for logging.
 */
int actor_pool_update(actor_spawn_pool *pool);

#endif /* OSS_ACTOR_SPAWN_H */
