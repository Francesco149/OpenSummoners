# Session handoff — rolling current state (last updated ckpt 61, 2026-06-04)

> **This is a ROLLING file — rewrite the current-state + next-move sections in place
> each checkpoint; do NOT append.** The dated per-checkpoint narrative is the
> append-only `PROGRESS.md` (every ckpt back to 26 is there); the 60-second front is
> `FRONT.md`; durable RE writeups are `findings/`. Keep this to: the current checkpoint,
> the next move, the module layout, and open RE threads.

## Where we are — ckpt 61

**The draw-node layer pool + the backdrop walk driver are ported + host-tested; the
decode → grid → geometry → draw-list chain is CLOSED.**

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
- **State:** **806 pass / 0 fail / 6 skip**. Ledger **189/1490 touched / 184 tested**
  (`0x586010` referenced by bare VA — only its layer-table slice is ported, so the 18 KB
  fn isn't over-counted). Both GUI builds clean; `draw_pool.c` in the `src` wildcard, not
  yet called by `main.c`. Full writeup: `findings/in-game-intro.md` "The draw-node layer
  pool + the backdrop walk driver".

## Next move
> The 60-second framing is in `FRONT.md`; this is the detail.

The **present pass** — walk the 27 layers, resolve each node's sprite, zdd-blit it (the
consumer that turns the draw list into pixels) — OR wire `map_decode` + `map_render_walk`
+ present into `main.c`'s in-game `game_drive` (today neither `game_map`/`game_world` nor
the new map modules are called by `main.c`) and diff vs `runs/tas-ingame-1` anchored on
`game_enter`. Port the **0x586010 sim step** only as far as needed to populate what the
backdrop reads (it allocs `DAT_008a9b50` 0x27b8), then a slice of **`0x5a00c0`** (13.7 KB;
reuses ported `ar_sprite_decode`/zdd/ramps). Target the static town backdrop FIRST
(golden flip ~1150). The **camera/view object construction stays a rock**
(`cam[0x34..0x74]` updated dynamically by gameplay scroll across many functions — no
clean pure init); host tests use synthetic cameras (window math is exact). Also register
the EXE-NULL banks `0x570-0x572` at the engine-time site once the slot indices surface.

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
**In-game (milestone 2 — pure + host-tested, NOT yet wired into `main.c`):**
game_drive (black map-load frame scaffold), game_world (registry + `0x585000` xref +
`0x561c90` lookup over generated `world_tables_data`), game_map (`0x59f2c0` fresh-entry
arm + `0x4c5350` `0x3f2`→room-210110 key), **map_data** (`0x587970` resource parse),
**map_grid** (runtime render grid + `0x54c970`/`0x58ca80`/`0x58c910` write primitives),
**map_decode** (`0x587e00` per-tile-id placement dispatch — the 9 town tile ids),
**map_render** (`0x490f30` geometry + `map_render_walk`), **draw_pool** (the 27-layer
draw-node pool `0x4917b0`/`0x586010`). Foundation the sim `0x586010` / render `0x5a00c0`
will read.

## How to run / verify live (self-serviceable — Frida host always up, UAC auto-approved)
```
# build (single-TU, full rebuild) + host suite, inside nix develop:
nix develop --command make -C src all && nix develop --command make -C tests run   # 806 pass / 0 fail / 6 skip

# capture port frames (BMPs land in the game dir = Windows C: drive):
cp build/opensummoners-debug.exe /tmp/oss.exe
./build/opensummoners-launcher.exe --timeout-ms 35000 -- /tmp/oss.exe \
    --hide-window --frames 2200 --capture-frames "60,200,400,700"
# then BMP->PNG (PIL, in nix develop) from /mnt/c/.../Fortune Summoners/port_frame_*.bmp
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
- **Camera/view object construction** — where `cam[0x34..0x74]` come from (updated
  dynamically by gameplay scroll across many functions; no clean pure init point).
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
