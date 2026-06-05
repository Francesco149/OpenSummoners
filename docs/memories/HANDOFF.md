# Session handoff — rolling current state (last updated ckpt 66, 2026-06-05)

> **This is a ROLLING file — rewrite the current-state + next-move sections in place
> each checkpoint; do NOT append.** The dated per-checkpoint narrative is the
> append-only `PROGRESS.md` (every ckpt back to 26 is there); the 60-second front is
> `FRONT.md`; durable RE writeups are `findings/`. Keep this to: the current checkpoint,
> the next move, the module layout, and open RE threads.

## Where we are — ckpt 67

**In-game COLOR-GRADE LUT ported → backdrop TILES are `differ_px==0`; the
"establishing shot" proven to be a PAN, not a zoom.**  Drove the menu→in-game
nav trace, diffed the port's town vs a *fresh new-trace* retail hold golden, and
chased the first divergence (the principled "stop at the first divergence, port
the missing thing" loop).

- **Establishing shot = leftward PAN at constant 1:1 scale** (overturns the
  ckpt-65/66 "zoom" framing).  Live-probed flips 1440–2100: viewport `+0x64/+0x68`
  and shear `+0x74` constant; only `+0x60` pans (128000 hold → 59450 by 2100).
  Free-roam render path every frame (`0x490cd0` fires; offscreen/special
  `0x499100`/`0x48c6b0` never); projector `0x490b90` has no scale term.  Port's
  static `MAP_RENDER_CAM_TOWN_3F2` aligns with the golden at **dx=0, same ~64px
  wall pitch**.  PORT-DEBT `ingame-establishing-zoom` **retired**.
- **The missing colour = an in-game per-channel tone-curve LUT** (`DAT_008a9410`),
  built by `FUN_00562ea0` (`0x5639fd-0x563a70`, a cosine curve over two config
  gates) and applied by `0x417c40` (parallax) + `0x490f30` (tiles); the
  title/menu/prologue use the plain getter `0x418470`, so they stay bit-exact.
  It is **NOT** the per-sprite tint (`DAT_008a93fc==0`, identity — ruled out
  live).  Builder **verified bit-exact** vs a live `DAT_008a9410` probe
  (`LUT[64]=35`/`128=100`/`192=175`); gates live-probed `gate1=700 gate2=850`.
  **PORT (`src/color_grade.{c,h}`, host-tested):** `color_grade_build_lut`
  (the formula) + `color_grade_apply_palette` (`0x417c40`'s per-channel RGBQUAD
  remap) + `color_grade_is_active`.  Wired in `main.c`: `enter_game` arms the
  grade before the town banks decode; `title_sheet_format` applies it to each
  **8bpp** sheet's palette *before* the 16bpp pack (retail's order → bit-exact,
  not LUT-after-565).  Scoped so the title sheets (converted earlier) stay
  identity.  **Result: the half-timber wall `(173,170,140)` and ivy
  `(107,105,74)` match retail exactly.**
- **RESIDUAL (open):** the **24bpp parallax** banks (`0x55`/`0x58`/`0x59`, sky+
  mountains) have no palette → the 8bpp grade skips them → the sky still renders
  too bright.  Retail must grade 24bpp by a different path (TBD) AND the port's
  24bpp→16bpp decode is itself brighter than retail's (port raw sky `132,186,255`
  vs back-solved retail raw `~103,165,231`).  PORT-DEBT `render-palette-tint`
  (sharpened: tile half done, 24bpp half + `color-grade-gates` derivation remain).
  Other residuals: NPC actors (blocked on entity/spawn), tree + "Town of
  Tonkiness" banner (`0x5a00c0`), the pan itself (`ingame-camera-snap`).
- **State (ckpt 67): 840 pass / 0 fail / 6 skip** (+4 color_grade).  Ledger
  **194/1490 touched / 189 tested** (+1: the `0x417c40` LUT slice is now
  host-tested).  Both GUI builds clean.  Full writeup:
  `findings/in-game-intro.md` "The in-game COLOR-GRADE LUT".

### (prior, ckpt 66) The PARALLAX far-plane

**PARALLAX FAR-PLANE landed (sky + mountain background).** On top of the ckpt-65
wired backdrop, the port now draws the **parallax far-plane** behind the tiles —
live-verified in-game (port `game_enter@1116`): frame 1200 shows the blue sky band
(layer A bank `0x55`) + the mountains (layers C/B banks `0x58`/`0x59`) under the
town tiles, where it was black before.

- **RE (two-witness, high confidence).** The background producer is `FUN_00490cd0`
  (inline; called FIRST in the per-frame world driver `0x48c150:47`, the free-roam
  path) and its twin `0x499100`→`FUN_00499560` (the establishing-shot/special path
  via `0x48c6b0`).  Both read the SAME 3-layer descriptor from the runtime grid's
  **front-header** (`*(DAT_008a9b50+0x1048)`) via select+blit `0x417c40`→`0x5b9a40`.
  The descriptor is written by the `0x587e00` PROLOGUE's `switch(param_2=room[0x44])`
  / `param_3=room[0x43]`; town (room 210110, area `0xd2`: A=4,C=1) → case 4 → A bank
  `0x55`; C bank `0x58` baseY `0xf8` wrap 8 paraY `0xfa` (0.5×); B bank `0x59` baseY
  `0xe0` wrap 8 paraY 0 (0.25×).  Full writeup: `findings/in-game-intro.md` "The
  PARALLAX far-plane".
- **PORT (pure, host-tested): `src/parallax.{c,h}`** — `parallax_select` (the
  prologue switch), `parallax_render`/`parallax_strip` (`0x490cd0`/`0x499560` math),
  `parallax_to_grid`/`_from_grid` (front-header bytes).  Wired into `town_render`
  (`town_render_parallax`, descriptor selected at load, drawn before the tilemap)
  and `main.c game_render` (sink = `game_parallax_blit` → `zdd_object_blt_onto`).
  9 host tests (8 `test_parallax.c` + 1 `town_render` wiring).
- **Fidelity boundary:** the port uses the plain frame getter `0x418470` (as the
  tiles do) where retail selects via the palette-aware `0x417c40` — the far-plane
  renders with the base palette (time/difficulty tint deferred, PORT-DEBT
  `render-palette-tint`).  Town params (4,1) are hardcoded in `town_render`
  (PORT-DEBT `ingame-nontile-layers`: derive from `game_map`/`game_world`).
- **LIVE-CONFIRMED bit-exact** (retail `--parallax-probe`, the re-synthesised
  trace, `game_enter@1433`): the descriptor `raw32` + the per-tile blit stream
  match the port's `parallax_render` byte-for-byte (incl. layer C y=220 = the
  clamped vertical parallax) → **data-1:1 at the producer**, `MAP_RENDER_CAM_TOWN_3F2`
  confirmed.
- **State (ckpt 66): 836 pass / 0 fail / 6 skip** (+9). Ledger **193/1490 touched /
  188 tested** (+2: `0x490cd0`, `0x499560`). Both GUI builds clean.

### (prior, ckpt 65) The wired backdrop

The backdrop pipeline is **composed (`town_render.{c,h}`) + WIRED into `main.c`**,
rendering the opening **town of Tonkiness backdrop** — the half-timbered house, the
vine trellis, the stone-block walls, ivy + grass — the **same assets at the matching
gameplay scale as the retail golden** (user-confirmed; cross-checked vs golden flip 1800).

- **The composition (pure, host-tested): `src/town_render.{c,h}`.** A thin
  per-room SCENE owning the shared state (parsed `map_data`, the runtime grid,
  the 27-layer `draw_pool`) run in engine order: `town_render_load` =
  `map_data_parse` (`0x587970`) + `map_decode` (`0x587e00` arms);
  `town_render_step` = the backdrop slice of the per-frame driver `0x48c150`
  (`draw_pool_reset` → `map_render_walk` `0x490f30` → `map_present` `0x48eac0`).
  6 host tests (`tests/test_town_render.c`).
- **The Win32 glue (`main.c`).** `load_town_scene(1022)` in `enter_game`:
  `LoadLibraryExA("sotes.exe", AS_DATAFILE)` → the EXE `.rsrc` (the engine-time
  module `DAT_008a6e7c`), `FindResource`/`Lock`(DATA 1022) + `town_render_load`.
  **Live-verified the packed `sotes.exe` `.rsrc` is readable** (Steam-DRM intact;
  no runtime Steamless): DATA 1022 = 152936 B "MSD_SOTES_MAPDATA" 88×19×3.
  The three engine globals are real callbacks: `game_sprite_resolve`
  (`ar_pool_get_slot(bank)` = `&DAT_008a760c[bank]` + `ar_sprite_slot_frame` =
  `0x418470`; bank→pool mapping verified: bank `0x62`→idx 85→res `0x433`, all
  town banks in g5), `game_bank_dims` (slot width/height), `game_present_blit`
  (mode-3 CLIPPED → `zdd_object_blt_clipped` `0x5b9bf0`). `game_render` clears
  black then walks `town_render` through `MAP_RENDER_CAM_TOWN_3F2`.
- **NOT `differ_px==0` yet — named residuals, ALL deferred layers (not logic):**
  the parallax sky/mountain far-plane + foreground trees + dialogue/caption
  overlay (`0x5a00c0`, PORT-DEBT `ingame-nontile-layers`); the NPC actors
  (present modes 0/1/2, PORT-DEBT `present-actor-modes`); retail's zoomed-out
  intro establishing shot at the hold (PORT-DEBT `ingame-establishing-zoom` — the
  camera scale field wasn't in the ckpt-64 probe); and the per-sprite palette
  tint (`render-palette-tint` — the "bit more color" the user noticed, the
  `DAT_008a93fc`/`0x4182d0` difficulty/time ramp, recolors pixels not geometry).
- **State (ckpt 65): 827 pass / 0 fail / 6 skip** (+6 town_render). Ledger
  **191/1490 touched / 186 tested** (pure composition, no new `FUN_`). Both GUI
  builds clean. The backdrop scene is now **driven by `main.c`** (the first
  in-game render module that is).

### (prior, ckpt 64) The camera/view object

- **RE — the camera IS the view object** (`view = *(room_state+0x104c)`, one
  `operator_new(0x78)` struct, allocated in the room-state ctor `0x4017d0:187`).
  Its room-entry init is clean + portable (`586010:854-872` sets viewport
  `+0x64=64000`/`+0x68=48000`, origins `+0x5c/+0x60/+0x74=0`; the two `587d30`
  calls zero the `+0x24`/`+0x3c` sub-blocks holding `+0x34`/`+0x4c`). So the
  ckpt-63 "dynamic-scroll rock, no clean pure init" framing is **refuted**.
- **Live ground truth (the harness, `src:"chain"` field-spec probe).** Added a
  global-deref field src (`*(*(0x8a9b50)+0x104c)+off`) + 9 `cam_*` fields to
  `retail_fields.json`; drove retail to the town twice (`--seed-pin --lockstep`).
  The camera **snaps to `+0x60=128000` (40 cells) / `+0x5c=12800` (4 cells) by
  flip 1093, holds ~83 flips through ~1176** (the town first renders ~1150,
  inside this hold), then runs a **scripted leftward pan** (~−300/flip cruise).
  Viewport matches the static init exactly → the 586010 RE is confirmed.
- **PORT (pure, host-tested):** `map_render_camera_init` (the room-entry zeroed-
  origin state) + the live-verified first-frame constant `MAP_RENDER_CAM_TOWN_3F2`
  (`+0x60=128000`, `+0x5c=12800`, vp 64000×48000; visible window cols 39-60 /
  rows 3-18), both in `src/map_render.{c,h}`. 2 tests. **DEFERRED** (PORT-DEBT
  `ingame-camera-snap`): the spawn-snap that derives the origin from the entry
  params + the intro pan (the dynamic-scroll engine across `0x4710c0`/`0x54f980`
  follow/copy + `0x499ab0`→`view+0x74`).

### (prior, ckpt 63) The in-game PRESENT PASS
**The decode → grid → geometry → draw-list → present chain is CLOSED.**

- **RE — `FUN_0048eac0` is the present pass; `FUN_00490b90` the shared projector.**
  The per-frame driver `0x48c150` resets the 27-layer table (`view+0x54` counts, ==
  `draw_pool_reset`), runs all the per-actor + tilemap emitters (`0x490f30` at :108),
  then calls **`0x48eac0`** to flush. `0x48eac0` walks the 27 layers in order (count at
  `view+0x58`); per node it dispatches on **mode (`+0x18`)** into 4 arms, each projecting
  the node's world pos to screen + culling, then blitting: **mode 0** → `0x5b9b70`,
  **mode 1** → `0x5bd550` (alpha), **mode 2** → the `DAT_008a9274`-palette scaled path,
  **mode 3** → `0x5b9bf0` (clipped color-key) when node `+0x14`==0 else `0x5bd550`. The
  shared projector **`0x490b90`** (used verbatim by modes 1/3, inlined by 0) computes
  `sx = wx/100 - (cam+0x60 + cam+0x34)/100 + offx`, `sy = wy/100 - (cam+0x5c +
  cam+0x74*100 + cam+0x4c)/100 + offy` and a four-corner cull vs `cam+0x64/100` /
  `cam+0x68/100`. `map_render_walk` emits **mode 3, param8=0** → the clipped path.
- **PORT (pure, host-tested):** `src/map_present.{c,h}` — `map_present_project` (`0x490b90`
  arg-for-arg) + `map_present` (the 27-layer walk + mode dispatch). **Mode 3 fully**
  (project w/ node w/h, select CLIPPED/ALPHA by `+0x14`); the cel handle in node `+0x00`
  → a `present_blit_fn` sink (the Win32 layer maps `PRESENT_CLIPPED`→`zdd_object_blt_clipped`,
  `PRESENT_ALPHA`→`zdd_blit_orchestrate`, both already in `zdd.c`). **DEFERRED**
  (PORT-DEBT `present-actor-modes`): modes 0/1/2 — VISITED in faithful order + counted via
  `out_deferred`, not blitted (no ported producer emits them; geometry reads engine sprite
  internals). 9 tests (`test_map_present.c`).

### (prior, ckpt 61) The draw-node layer pool + the backdrop walk driver

- **RE — the layer table is one structure shared by `0x4917b0` + `0x586010`.**
  `FUN_00490f30`'s `0x4917b0` enqueue writes into the render context's DRAW-NODE TABLE at
  **`view + 0x54`** (`view = *(room_state + 0x104c)` — the object `0x490f30` takes as
  `param_1`). `0x586010:510-650` builds it: `operator_new(0xd8)` = **27 (`0x1b`) 8-byte
  layer slots** `{u16 count, u16 cap, ptr node[cap]}`, each given its own
  `operator_new(cap*0x3c)` node array; the 27 caps are literal-stamped (layer1=`0x80`,
  layer2/3=`0x1b8`, layer6=`0x400`, …). **Slot 0 is never given an array (cap 0) → every
  emit to layer 0 fails** — a real quirk, preserved. Present walks the 27 layers in order,
  so the layer index = the draw-order key. `0x4917b0` (106 B) = per-layer bump alloc:
  `node = layer[key & 0xffff]; if (cap <= count) return 0;` else stamp 6 caller dwords
  (`+0x00` sprite, `+0x04/+0x08` dst, `+0x0c/+0x10/+0x14` aux, `+0x18` mode), bump
  `count`, return the node for `490f30` to finish (`+0x2c..+0x38` src rect); the
  `CONCAT22` high-word sort key is masked off (dead in the allocator).
- **PORT (pure, host-tested):** `src/draw_pool.{c,h}` — `draw_pool_init`/`_reset`/`_free`
  (the 27-layer table; `draw_pool_default_caps[]` verbatim) + `draw_pool_emit`
  (`0x4917b0` arg-for-arg; node is exactly 0x3c B, asserted). `map_render_walk` (added to
  `map_render.{c,h}`) — the backdrop-tile core of `490f30.c:55-229`: visible window,
  scan rows-outer/cols-inner, per populated region-A sub-slot resolve the sprite +
  `draw_pool_emit` a node (layer = region-A `+0x4`, mode 3, dst = tile world origin,
  `0x20×0x20` src rect). The sprite manager (`0x418470`/`&DAT_008a760c`) is an
  `mr_sprite_fn` **callback** so the walk stays pure; tile skipped when the resolver
  returns 0. **DEFERRED:** palette tint (`DAT_008a93fc`/`0x4182d0`) + the region-C
  blend/overlay arms (`0x1b58d`/`0x1b5ab`, `490f30.c:230-282`) — registered in
  `port-debt.md`.
  (`0x586010` referenced by bare VA — only its layer-table slice is ported, so the 18 KB
  fn isn't over-counted.) Full writeup: `findings/in-game-intro.md` "The draw-node layer
  pool + the backdrop walk driver".
- **State (ckpt 64):** **821 pass / 0 fail / 6 skip** (+2: camera init + first-frame).
  Ledger **191/1490 touched / 186 tested** (unchanged — the camera init is a slice of the
  bare-VA-referenced `586010`/`587d30`, no new `FUN_` token). Both GUI builds clean; all
  the in-game render modules are in the `src` wildcard but **not yet called by `main.c`**.

## Next move
> The 60-second framing is in `FRONT.md`; this is the detail.

**Tiles are bit-exact + colour-graded; finish the colour, then build out content.**
The smallest visible wins, in order:
1. **The 24bpp parallax colour** (`render-palette-tint`, the open half). The sky/
   mountain banks (`0x55`/`0x58`/`0x59`) are 24bpp → no palette → the 8bpp grade
   skips them → the sky renders un-graded/too-bright. Two sub-tasks: (a) RE where
   retail grades 24bpp banks (the `0x417c40` palette loop is 8bpp-only; find the
   24bpp path / whether `0x490cd0`'s `0x5b9a40` blit applies it), and (b) reconcile
   the port's 24bpp→16bpp decode being brighter than retail (port raw sky
   `132,186,255` vs back-solved retail raw `~103,165,231`). Once both land, apply
   the LUT to 24bpp pixels (preserving the colour-key) and the sky matches.
2. **The actor renderers** (`0x491ae0` et al.) → present **modes 0/1/2** (the NPCs).
   **BLOCKED:** `0x491ae0` reads a fully-populated entity object from the actor
   pools off `DAT_008a9b50` — the entity/spawn system isn't ported yet (the upstream
   `0x59f2c0` 8-slot actor init + `FUN_00560e60`). Port that foundation first.
3. **The "Town of Tonkiness" banner + the foreground tree** (`0x5a00c0`, the
   scripted-scene overlay player + the `DAT_008a7640` font bank) — PORT-DEBT
   `ingame-nontile-layers`.
4. **The intro PAN** (`ingame-camera-snap`) — animate `+0x60` (128000 hold →
   ~−147/flip from ~flip +167) so port + golden share the camera across the shot,
   unlocking a flip-anchored full-frame diff (the camera is now proven: pan, dx=0).

**HARNESS — in-game retail drive RESTORED (ckpt 66).** The old `trace-retail.jsonl`
had gone stale (retail's title turns interactive ~150 flips later than it used to, so
the old `Start@615` was eaten and retail sat on the title). Re-synthesised a working
trace (confirm-spam 600..760 → new-game; down×2+confirm → prologue; Z-beats → in-game):
VERIFIED `newgame_enter@750 / prologue_enter@945 / game_enter@1433`. The
`--parallax-probe` then live-confirmed the parallax descriptor + blit stream **bit-exact**
vs the port (see the parallax section). Caveat: the working trace tolerates a stray
confirm landing on the difficulty menu (the down×2 recovers); robust across 3 runs.
NOTE (separate, user-reported): the **PORT** does not take **real keyboard** input when
run interactively (arrows/Enter do nothing) — only `--input-trace` replay drives it;
the windowed DInput/`GetDeviceState` path needs fixing (next task).

**Before a flip-anchored full-frame diff** vs `runs/tas-ingame-1`: pin the
**establishing-shot/zoom** relationship (PORT-DEBT `ingame-establishing-zoom`).
Retail's flip-1150 hold is a zoomed-OUT vista that zooms to 1:1 by ~1800; the
port renders gameplay 1:1 at the hold's scroll origin. So port + golden don't
share a camera at any single flip yet — the backdrop tiles are confirmed by ASSET
+ SCALE match (vs golden 1800), not by a px-exact frame diff. Find the view scale
field (or the `0x5a00c0` overlay projection) that drives the establishing shot.

How to drive the port in-game live: `--input-trace
tests/scenarios/in-game-intro/trace-port.jsonl --frames 1400` (copy the trace into
the game-dir CWD; `game_enter@1116`), `--capture-frames "1160,1200,1300"` → BMPs
in the game dir → PNG → feed. The backdrop renders from `game_enter` (the entry
fade/black-load timing is deferred).

## Module inventory — render + text pipelines complete; in-game data layer ported (not wired)
**Title/menu shell (bit-exact):** pixel_drawer, asset_register, bitmap_session, wnd_proc,
zdd, cs_dispatch, app_pump, title_scene (`0x56aea0`), input (`0x43c110`), obj_container,
menu_list, title_render, title_sink, title_drive, rng (LCG `0x5bf505`/`_5bf4fb`),
title_particles (phase-7 sparkle), app_flow (post-title dispatch).
**Text pipeline (bit-exact):** glyph_text (`0x40fa00`/`0x40fd20` layout builder),
glyph_render (`0x48e200` GDI render), glyph_wrap (`0x40e5e0`/`0x40f040` tooltip
word-wrap).
**New-game config scene (bit-exact + user-confirmed):** newgame_menu (`0x564780` case
0x24 builder), newgame_scene (run-loop model), newgame_box (`0x48cf80` 9-slice panel),
newgame_cursor (`0x48d940` selection cursor), newgame_picker (`0x567ba0` option submenu).
**In-game (milestone 2 — pure + host-tested; the backdrop chain is now WIRED into
`main.c` via `town_render`, ckpt 65):**
game_drive (the in-game run-loop shell), **town_render** (composes the backdrop:
`map_data_parse`+`map_decode` load → `draw_pool_reset`+`map_render_walk`+`map_present`
step + `town_render_parallax` → `parallax_render` — driven by `main.c game_render`),
**parallax** (the sky/mountain far-plane `0x490cd0`/`0x499560` + the `0x587e00`-prologue
bank-selection `parallax_select`, ckpt 66), game_world (registry + `0x585000` xref +
`0x561c90` lookup over generated `world_tables_data`), game_map (`0x59f2c0` fresh-entry
arm + `0x4c5350` `0x3f2`→room-210110 key), **map_data** (`0x587970` resource parse),
**map_grid** (runtime render grid + `0x54c970`/`0x58ca80`/`0x58c910` write primitives),
**map_decode** (`0x587e00` per-tile-id placement dispatch — the 9 town tile ids),
**map_render** (`0x490f30` geometry + `map_render_walk` + the camera init `586010:854-872`
/ first-frame constant `MAP_RENDER_CAM_TOWN_3F2`), **draw_pool** (the 27-layer
draw-node pool `0x4917b0`/`0x586010`), **map_present** (`0x48eac0` 27-layer flush +
projector `0x490b90`, mode-3 backdrop path → ported zdd blits). The decode → grid →
geometry → draw-list → present chain is complete + the camera is RE'd; the `0x586010` sim
slice + `main.c` wiring (sprite resolver, EXE-NULL banks) are what remain to drive it with
real data.

## Tooling — Phase B B2 (field-bearing flow trace) LANDED 2026-06-05
The LOGIC drill-in is built + **live-verified on retail** (`docs/plans/trace-tooling-phase-b.md`):
- `src/call_trace.{c,h}`: `seq` (per-frame exec order) + `CALL_TRACE_BEGIN/FIELD/END`
  (`I32/U32/F32/HEX`, + `_STUB`). `tools/flow/retail_fields.json` is the retail spec; the
  Frida agent reads `src: global|arg|argderef` (`retval` = onLeave TODO) into `f:{…}` with a
  per-Flip seq; `frida_capture.py --field-spec[-only]` auto-hooks spec VAs (bounded mode).
  `tools/flow_diff.py` (+ `test_flow_diff.py`, 9 tests) names the first `[chain]`/`[data]`
  divergence; `--field-timeline` localizes per-field state drift.
- **First probe:** `rng` (`DAT_008a4f94`) at the **Flip `0x5b8fc0`** — the shared once-per-
  frame VA. The title runner `FUN_0056aea0` keeps its loop INTERNAL (onEnter once, not per
  frame) so it was the wrong join VA; the Flip is right.
- **NUANCE (next session, don't trip on it):** the port emits `0x5b8fc0` from *two* sites —
  `src/main.c drive_present` (the rng `BEGIN`, runs every frame) and `src/zdd.c:894`
  `CALL_TRACE_ENTER` (the real `zdd_present`, the bare call-coverage probe). Under
  `--hide-window` (always used for parity) `zdd_present` is SKIPPED, so only the rng probe
  fires → clean 1 row/frame. A *non*-hidden run would show 2 rows/frame at `0x5b8fc0`.
- **First result:** title-sparkle RNG is **data-1:1** (port & retail both land on
  `0x404a0a8f`); the per-flip divergence is the R3 title-pace (phase) skew — port anchor
  `subtitle_anim_start` @flip 437 vs retail @897, sparkle compressed into fewer port flips.
  Not a logic bug. Anchor+rate (pace-aware) alignment is the refinement when chased to px.

## How to run / verify live (self-serviceable — Frida host always up, UAC auto-approved)
```
# build (single-TU, full rebuild) + host suite, inside nix develop:
nix develop --command make -C src all && nix develop --command make -C tests run   # 806 pass / 0 fail / 6 skip

# capture port frames (BMPs land in the game dir = Windows C: drive):
cp build/opensummoners-debug.exe /tmp/oss.exe
./build/opensummoners-launcher.exe --timeout-ms 35000 -- /tmp/oss.exe \
    --hide-window --frames 2200 --capture-frames "60,200,400,700"
# then BMP->PNG (PIL, in nix develop) from /mnt/c/.../Fortune Summoners/port_frame_*.bmp

# B2 field-bearing flow trace (the LOGIC drill-in) — retail + port, then diff:
#   retail (bounded: hook ONLY the field-spec VAs, use ABSOLUTE /tmp paths):
OPENSUMMONERS_DURATION_MS=35000 nix develop --command bash tools/run-retail.sh \
    --no-turbo --hide-window --seed-pin --call-trace --field-spec-only \
    --call-trace-frames 900,950,1000,1050 --run-dir /tmp/b2live --exact-run-dir
#   port (drive_present emits rng at the Flip 0x5b8fc0 every frame under --hide-window):
./build/opensummoners-launcher.exe --timeout-ms 80000 -- /tmp/oss.exe \
    --hide-window --frames 1200 --call-trace /tmp/port_ct.jsonl --call-trace-frames 900,950,1000,1050
#   diff (default = per-frame seq-aligned chain+data walk; --field-timeline = per-field):
nix develop --command python3 tools/flow_diff.py \
    --retail /tmp/b2live/call_trace.jsonl --port /tmp/port_ct.jsonl --all
# NB align on an ANCHOR not the raw flip index (title-pace skew: port anim_start@437 vs
# retail@897) — the rng field is data-1:1 (both end 0x404a0a8f) under correct alignment.
```
NB Flip frames advance ~1 per 2 main-loop iterations (pace split), so reaching Flip 700
needs a generous `--frames`/timeout. Retail-side capture + the anchor-aligned pixel diff:
`docs/parity-harness.md`. The per-ckpt probe flags (`--cursor-probe`/`--fade-probe`/
`--pace-probe`/`--seed-pin`/`--textout-probe`/`--menu-trace`) are catalogued in
`PROGRESS.md` and get folded into the unified `scenario-test.py` in the Phase-B harness
work (`docs/plans/`).

## Open RE threads (see ROADMAP subsystem map for the rest)
- **The render rock's deferred arms** (`port-debt.md`): the sprite-resolve palette tint
  (`0x4182d0`), the region-C blend/overlay arms (`0x1b58d`/`0x1b5ab`), the `0x587e00`
  prologue (front-header flags + HUD/border bank selection + `0x1bd82` autotile) + its
  trailing layer pass (`0x58c8c0`/`0x58c8d0`/`0x58cb30`).
- **Camera/view object** — RESOLVED for the static first frame (ckpt 64): the object +
  its room-entry init + the live-probed first-frame value are RE'd/ported (`map_render`
  `MAP_RENDER_CAM_TOWN_3F2`). STILL OPEN (PORT-DEBT `ingame-camera-snap`): the spawn-snap
  that derives `+0x60`/`+0x5c` from the entry params, and the scripted intro pan (the
  dynamic-scroll follow across `0x4710c0`/`0x54f980` + `0x499ab0`→`view+0x74`).
- **Register batches not yet called at boot:** `ar_register_fonts`,
  `ar_register_palette_ramps` (`0x57a330`), the big `0x56e190` (442 sprites), sounds —
  the in-game/prologue scenes need them (all take the sotesd HMODULE).
- **Audio ZDM** `0x5bab10`/`0x5bc150` + SFX `0x411390` — milestone 3.
- **Launcher `config.dat`** `0x5a4770` (46 KB) — milestone 4 (loads sotesd/w/p.dll,
  handles at `DAT_008a6e74/78/7c`).
- **Input producer** (DInput `GetDeviceState`, vtable `[0x24]`) + axis-held flags — black
  box; `mem_watch.py` is the tool.
- **God-object `DAT_008a9b50`/`DAT_008a6e80` layout** — model as we go.

## How to apply (when the user says "continue RE work" or similar)
1. Read `FRONT.md` (60-sec) then this file; `STATUS.md` + `ROADMAP.md` for coverage/next.
2. Pick the recommended next move (or whichever the user redirects to).
3. Port-and-test: small unit → host test → commit. Each ported function gets a
   `FUN_XXXXXX` provenance comment; pin retail offsets via `_Static_assert`. **Reference
   UNPORTED callees by bare VA, never `FUN_`** (it inflates the ledger).
4. **Append any engine quirk** to `findings/engine-quirks.md` (retail behavior only).
   Tag any MVP/synthetic shortcut `PORT-DEBT(...)` + a row in `port-debt.md`.
5. **Regen** `gen_port_ledger.py` + `gen_frontier.py` after a port; check the headline
   didn't move unexpectedly.
6. Verify rendering with `--capture-frames` vs goldens; bit-exact bar (`parity-model.md`).
7. Update `FRONT.md` + this file each meaningful checkpoint; append to `PROGRESS.md`.
8. Suggest a `/clear` at the natural stop point.
