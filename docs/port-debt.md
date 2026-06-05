# Port-debt registry

**Full port, not MVP.** Every synthetic / MVP / "good enough for now" shortcut that could
**silently cap 1:1 parity** is registered here with a retire condition, so a fake never
hides behind a green test suite. This is the OpenSummoners analog of openrecet's
`port-debt.md`.

## The convention
- When you take a shortcut that diverges from retail behavior (a synthetic value, a
  skipped arm, a zero-init where retail does something, a constant where retail computes),
  add an inline **`PORT-DEBT(tag): …`** comment at the site AND a row in the table below.
- The `tag` is a short kebab-case slug, unique, cited in the row and the code.
- A row needs: **what's faked**, **why** (why it was safe to defer), and the **retire
  condition** (the concrete event that forces it to be made faithful — usually "when
  subsystem X is ported" or "before scene Y can be bit-exact").
- Retire by porting the real behavior, then delete the comment + the row.

## NOT port debt (don't register these)
The host-build vs Win32-build split is **by design**, not debt: `*_win32.c` real impls
with recording stubs in the host test harness, and `calloc`-for-determinism where retail's
`operator_new` leaves bytes uninitialised that are then always explicitly written (same
observable result). Only register a zero-init/stub when it produces a behavior that
**differs from retail's observable output**.

## Open debt

| tag | what's faked | why safe for now | retire when |
|-----|--------------|------------------|-------------|
| `map-zero-init` | `game_map.c`: the `0x4120` map object is zero-initialised; retail's `operator_new` is raw + filled by opaque sub-inits (`0x5612b0`/`0x5611d0`/`0x4e59a0`). The `+0x4020` ramp ceiling is one of those outputs, so the `+0x4014` ramp is inert under zero-init. | The verified room-key path (`+0x4024 = 0x334be` → room 210110) doesn't depend on the ramp. | the in-game sim reads `+0x4014`/`+0x4020` for anything visible. |
| `map-blend-va` | `map_decode`: the two `0x1b58d` blend pointers `&DAT_005cc410`/`&DAT_005cc430` are stored verbatim as their retail VAs (`MD_BLEND_*`) in region B `+0x8`. | The render port hasn't consumed them yet; storing the raw VA preserves the exact bytes for later translation. | the render port dereferences region B `+0x8` (translate VA → ported blend object). |
| `map-emit-tile-params78` | `map_grid_emit_tile` drops decompiled params 7/8 of `0x58c910` (see `map_grid.c`). | Unused by the 9 town tile ids exercised by DATA 1022. | a map/tile id that passes non-default params 7/8 is decoded. |
| `render-palette-tint` | `map_render`/`draw_pool`: the per-sprite palette tint (`DAT_008a93fc`/`0x4182d0`) is not applied — geometry only. | Recolors pixels, not draw-list geometry; the geometry is provable without it. | the present pass blits actual pixels and the tint affects the diff. |
| `render-regionC-blend` | `map_render`: the region-C blend/overlay arms (`0x1b58d`/`0x1b5ab`, `490f30.c:230-282`) are not walked. | The town backdrop's base tiles render without them; isolate first. | the backdrop diff shows the blend/overlay objects. |
| `present-actor-modes` | `map_present` (`0x48eac0`): node modes 0/1/2 (the actor/sprite/scaled draws — `FUN_005b9b70` / `FUN_005bd550` / the `DAT_008a9274`-palette scaled path) are VISITED but not blitted; only mode 3 (the backdrop tile path) is presented. Reported via `out_deferred`, never silently dropped. | No ported producer emits modes 0/1/2 yet (the only wired producer is `map_render_walk`, mode 3); their geometry reads engine sprite/paint_ctx internals (cull dims `sprite+0x1c/+0x20`, the palette table). | the actor renderers (`0x491ae0` et al.) are ported and emit non-tile nodes. |
| `decode-prologue` | `map_decode`: `0x587e00`'s prologue (front-header flags + HUD/border bank selection over `DAT_008a76xx` + the `0x1bd82` autotile pre-pass) and trailing layer pass (`0x58c8c0`/`0x58c8d0`/`0x58cb30`) are deferred. | Dead code for the opening town (every town tile id `< 0x1bd82`; no HUD/border in DATA 1022). | a map that exercises HUD/border families, autotile, or the layer pass is decoded. |
| `ingame-camera-snap` | `map_render`: the opening-town first-frame camera is the live-probed constant `MAP_RENDER_CAM_TOWN_3F2` (`+0x60=128000`, `+0x5c=12800`); the engine spawn-snap that derives the origin from the entry params (`+0x4028`/`+0x402c` via `FUN_004c5350`) and the scripted intro pan (the dynamic-scroll engine: `0x4710c0`/`0x54f980` follow/copy + `0x499ab0`→`view+0x74`) are unported. `map_render_camera_init` ports the *room-entry* zeroed-origin state (`586010:854-872`); the snap that moves it before frame 1 is faked by the constant. | The first town frame (~flip 1150) sits in the stable hold before the pan; the constant is byte-verified against retail at that frame. | the backdrop must track the camera across the pan (>~flip 1176) or match another room's spawn. |
| `exe-null-banks` | The EXE-NULL sprite banks `0x570-0x572` (registered with `settings=NULL`, sourced from `sotes.exe` as a datafile) are not yet registered at the engine-time site. | The slot indices haven't surfaced; the backdrop's base banks load lazily. | the in-game backdrop references a `0x570-0x572` bank (decode diverges without it). |
| `ingame-nontile-layers` | `main.c game_render`: only the mid-plane TILE grid (`map_render_walk` mode-3 nodes) is drawn for the town.  The full-screen **parallax sky/mountain far-plane**, the **foreground tree layer**, and the **dialogue box + caption overlay** (`0x5a00c0`, the scripted-scene player with its own draw-list + the `DAT_008a7640` font bank) are not drawn — so the upper/right of the frame is black where those layers belong (live capture, in-game-intro). | The backdrop tile layer is provable + asset/scale-correct standalone (cross-checked vs golden flip 1800); the non-tile layers are separate producers. | the actor renderers + `0x5a00c0` slice are ported (this row splits into actor / far-plane / dialogue as each lands). |
| `ingame-establishing-zoom` | `main.c game_render` renders the town at gameplay 1:1 (32 px/tile) at the `MAP_RENDER_CAM_TOWN_3F2` scroll origin; retail's flip-1150 hold shows a **zoomed-OUT establishing shot** (whole-town vista + banner) that zooms to 1:1 by ~flip 1800.  The camera **scale/zoom field** that drives the establishing shot was not in the ckpt-64 probe (which read only scroll `+0x34..+0x74` + viewport `+0x64/+0x68`). | The gameplay-scale backdrop is correct (golden 1800 matches the port's tile scale); only the intro establishing-shot/zoom differs, a render-time cutscene effect. | the establishing-shot zoom (a view scale field or a `0x5a00c0` overlay projection) is RE'd + the on-screen relationship to `MAP_RENDER_CAM_TOWN_3F2` is pinned. |
| `title-loop-sidefx` | `title_scene` outer-loop side-effect hooks are stubbed: `0x5b1030` (message pump — replaced by the harness pacing), `0x43e140`/`0x40fe00` (audio/joystick update), `0x43c2e0` (per-entry child-widget anim). | None are the intro-pacing key (that was the driving cadence, fixed in `main_loop_body`); the title is bit-exact without them. | audio (milestone 3), joystick input, or in-row sub-widget animation lands. |
| `newgame-fadeout-render` | `newgame_drive`: the new-game→prologue fade-out re-renders the menu **opaque** for ~20 frames; retail fades the box-panel alpha (`0x48cf80` alpha arm via `0x5bd550`) + the GDI menu text. Transition *timing* is aligned (ckpt 48); the *render* is not bit-exact. | Self-contained polish; the timing (the load-bearing part) is correct. | the ~20 transition frames (port ~801-820) must be bit-exact vs retail's ~795-814 window. |
| `newgame-box-fadein` | `newgame_box`/`newgame_cursor`: the box fade-in (`0x48cf80` alpha arm) is not ported — the box pops in opaque. | Cosmetic; the settled box is bit-exact. | the fade-in frames are diffed. |
| `glyph-escape-expander` | `glyph_text`: the escape expander (`0x4034f0`/`0x4051d0`) is hooked NULL; the sprite-cell render mode (`0x48e200` `param_1==0`) and SJIS kinsoku word-wrap (`glyph_wrap` cls 3) are deferred. | English labels carry no escapes / SJIS; ASCII path is exact. | a string with an escape sequence, a sprite-cell, or Japanese text is rendered. |
| `assetreg-clone-defer` | `asset_register`: 94 `SS_MGR` clone calls + the 9 inline-clone clusters are deferred; some entries are observability-only placeholders pending extracted PE bytes. | The title/menu/new-game/prologue paths don't need the deferred sprite clones. | a scene that needs a deferred clone (the diff shows a missing sprite). |

## Verification gaps (not synthetic, but parity not yet *proven*)
| tag | gap | unblock |
|-----|-----|---------|
| `picker-flip-gate` | `newgame_picker` render can't be flip-diffed: retail's flip counter freezes inside the `0x565d10` modal pump, so flip-keyed capture/inject see only the title→menu transition. | a non-flip-keyed capture path (a Phase-B `scenario-test.py` anchor inside the modal pump). |
