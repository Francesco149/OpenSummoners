# Session handoff — rolling current state (last updated ckpt 63, 2026-06-05)

> **This is a ROLLING file — rewrite the current-state + next-move sections in place
> each checkpoint; do NOT append.** The dated per-checkpoint narrative is the
> append-only `PROGRESS.md` (every ckpt back to 26 is there); the 60-second front is
> `FRONT.md`; durable RE writeups are `findings/`. Keep this to: the current checkpoint,
> the next move, the module layout, and open RE threads.

## Where we are — ckpt 63

**The in-game PRESENT PASS is ported + host-tested; the
decode → grid → geometry → draw-list → present chain is CLOSED.**

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
- **State (ckpt 63):** **819 pass / 0 fail / 6 skip**. Ledger **191/1490 touched / 186
  tested** (+2 this ckpt: `0x48eac0`, `0x490b90`). Both GUI builds clean; all the
  in-game render modules are in the `src` wildcard but **not yet called by `main.c`**.

## Next move
> The 60-second framing is in `FRONT.md`; this is the detail.

**Real town-backdrop pixels.** The pure pipeline is complete end-to-end (decode → grid →
geometry → draw-list → present); two blockers remain before the backdrop renders:
**(1) the camera/view object** (`cam[0x34..0x74]`) — a dynamic-scroll rock with no clean
pure init (host tests use synthetic cameras; window + projector math is exact), and
**(2) wiring into `main.c`** — today neither `game_map`/`game_world` nor the map
render/present modules are called by `main.c`; the in-game `game_render` clears to black.
The wiring needs the **`0x586010` sim step** ported only as far as it populates what the
backdrop reads (it allocs `DAT_008a9b50` 0x27b8 + builds the layer table at `view+0x54`
+ resolves the map data) plus a real **sprite resolver** (`0x418470` / `&DAT_008a760c`,
the `mr_sprite_fn` the walk takes as a callback) and the **EXE-NULL banks `0x570-0x572`**.
Once those exist, run `map_decode` → `map_render_walk` → `map_present(sink=zdd)` into
`game_render` and diff vs `runs/tas-ingame-1` anchored on `game_enter`. Target the static
town backdrop FIRST (golden flip ~1150). (The older "slice of **`0x5a00c0`**" framing was
corrected at ckpt 60 — `0x5a00c0` is the scripted-scene overlay player, not the tilemap;
the tilemap is `0x490f30` → present `0x48eac0`, both now ported.)

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
draw-node pool `0x4917b0`/`0x586010`), **map_present** (`0x48eac0` 27-layer flush +
projector `0x490b90`, mode-3 backdrop path → ported zdd blits). The decode → grid →
geometry → draw-list → present chain is complete; the sim `0x586010` + camera build are
what remain to drive it with real data.

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
