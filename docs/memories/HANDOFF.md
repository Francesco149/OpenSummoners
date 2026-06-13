# Session handoff — rolling current state (last updated ckpt 131, 2026-06-13)

> **This is a ROLLING file — rewrite the current-state + next-move sections in place
> each checkpoint; do NOT append.** The dated per-checkpoint narrative is the
> append-only `PROGRESS.md` (every ckpt back to 26 is there); the 60-second front is
> `FRONT.md`; durable RE writeups are `findings/`. Keep this to: the current checkpoint,
> the next move, the module layout, and open RE threads.

## Where we are — ckpt 131

**The USER drove a frame-by-frame trace-studio pass to close the dialogue-rendering gaps up to the errands
(directive: "we do not go any further until this is perfect 1:1").  FOUR bugs fixed + harness-verified; the
dialogue BOX POSITION is the one remaining 1:1 gap (USER chose the faithful fix).**  Commits `e8d5c0b`
(errands floor) / `1d826c3` (props) / `cbbab94` (portrait offset); 1010 host pass (+1).

**Fixed this ckpt:**
- **Errands bottom floor** — the floor tileset bank 0x188 was boot-cloned from `0x186` (res 0x76b, a
  1-frame 32×32 sprite), so its floor tiles (frames 4/7/11 at row y=13) were f_38-culled.  Frida pool read
  (`tools/flow/errands_tileset_fields.json`): retail bank 0x187=res 0x769 f_38=24, 0x188=res 0x76a f_38=16
  = the town floor banks 0x184/0x185 (area-0xd2 rooms share the floor sheets).  Fixed the inline-clone
  sources {0x187←0x184}/{0x188←0x185}; +1 test.
- **House + errands props** — the STRUCTURE band (map-driven, quirk #84) now re-spawns per room
  (`reload_room_backdrop` → `actor_spawn_struct_from_map`, house 21 / errands 58) and `game_actor_walk`
  runs for every room (STRUCTURE always; the town cast bands gated on `room_is_town`).
- **Dialogue portraits — the +13 pool/array offset (EVERY bust wrong).**  The render indexed
  `g_ar_sprite_slots[pslot]` directly; the 0x49d6e0 face table returns a POOL index → must go through
  `ar_pool_get_slot` (−13, the ramp slots).  So every portrait was shifted +13 → wrong bust resource (right
  character, wrong expression/variant) + wrong dims (var-B 176×144 vs 160×176).  Frida-verified: retail pool
  676=0x7ef / 704=0x7f9 / 460=0x5a3 live at the port's slots 663/691/447.  Fix: `ar_pool_get_slot(pslot)`;
  port now emits retail's exact slots + 160×176.

**NEXT MOVE — the dialogue box POSITION (the remaining 1:1 gap, USER-chose the faithful fix):** retail
anchors the box to the SPEAKER's projected screen position (`0x49c640`: project the speaker body world pos
via `0x490b90`, center the box, clamp), so the box moves to whoever speaks (Father box≈(174,148),
Arche≈(94,160), Mother≈(62,148)).  The port hardcodes `DIALOGUE_BOX_X=174/Y=148` (`dialogue.h`).  The
arrival cast is largely CLUSTERED/static during the dialogue (early-vs-late frames match, slight drift), so:
port `0x49c640` over the cast's positions + the port projector → drive `box_x/box_y` per speaker in
`game_render_dialogue`; refine with the walk-in cast only if the drift matters (`cutscene-party-chars`).
Ground truth: `tools/flow/portrait_pos_fields.json` reads the live box obj `+0xc/+0x10` per line (hook
0x49c910, param_1=this+0x54, *param_1=box obj; `0x49c640` decompile in `by-address/49c640.c`).  TODO before
this: locate the port's cast actor world positions + the world→screen projector; map the dialogue speaker
(name → cast code 0xc3dc/0xc35a/0xc440) → actor.

**Studio gap (build it):** the NOTE/mark UI is dual-mode only — single-file scrub (`osr_view.exe <one.osr>`)
has no notes panel.  The USER flagged frames directly (port-rooms.osr 4103=L3 Mother / 5165=L5 Arche) which
worked, but the mark→`notes.py` round-trip needs single-file support.

**Module layout (this ckpt):** `src/asset_register.c` (the inline-clone fix + the f_38 guard from ckpt 130),
`src/main.c` (`game_actor_walk` per-room STRUCTURE + the portrait `ar_pool_get_slot` fix +
`reload_room_backdrop` struct re-spawn), `tests/test_asset_register.c` (+`inline_clones_errands_floor_tilesets`).
Specs: `tools/flow/{errands_tileset,portrait_slot_dims,portrait_pos}_fields.json`.  Artifacts (gitignored):
`runs/{errands-tileset-gt,portrait-slot-dims,portrait-pos,speaker-pos}`; `C:\oss-osr\port-rooms.osr` (the
fixed full capture) + `port-dlg.osr` (arrival).

**OPEN RE threads:** the box-position arc above; the errands CHARACTER-band shop items + room cast (per-room
sprite tables, `actor-sprite-table`); carried — the freeroam hand-off, the errands questline 0x4dc510.

## Where we were — ckpt 130

**ROOM-RENDER LANDS — the house (DATA 1023) + errands/freeroam (DATA 1025) ROOM BACKDROPS RENDER.**  The
paused movement-system arc resumes (trace-studio v2 unblocked it): the town-intro cutscene's room-key swap
(arrival `0x334be` → house `0x334c8` → errands `0x334dc`) now RELOADS the backdrop per room, the per-room
`map_decode` tilesets are ported BIT-EXACT, and both interior/freeroam rooms render their real backdrops
(montages on the feed).  **5 commits** (`c2b1568` M1, `e228150` M2, `c3accc0` M3, `87fafd5` tooling,
`87bf668` M4); **1009 host pass** (+5).  The **main-goal room (the errands/freeroam scene) renders**; the
controllable-Arche hand-off is the next arc.

**What landed (M1–M4):**
- **M1 — scene + parallax from the active room.**  `game_world_room_render_cfg(w, key, &scene, &p2, &p3)`
  resolves a room key to its DATA scene (`GW_ROOM_SCENE`) + the 0x587e00 prologue params (param_2=room[0x44],
  param_3≈room[0x43]); `town_render_load` takes `(parallax_p2, parallax_p3)`; `main.c load_room(key)` builds
  the registry (lazy) + loads.  `enter_game` → `load_room(0x334be)`.  Byte-verified: arrival→1022/(4,1),
  house→1023/(4,1), errands→1025/(9,4).
- **M2 — the house + errands map_decode arms, BIT-EXACT to a retail emit capture (the "main RISK" RE).**
  `map_decode_cfg` + `map_decode_cfg_init(param_3, param_4)` port the 0x587e00 prologue's tileset-bank
  selection (param_4=room[0x43] switch → local_1c/18/24/20) + the param_3 scene-frame normalization.  14 new
  tile-id arms: dir6 family generalized (0x29ffe→0x178/0x29c02→0x190/0x29c0c→0x191), 0x1b59f, 0x1b5b3,
  0x2724/0x2738 (block, base 0x5d), 0x272e (block w/ blends, base 0x60), 0x1b986/0x1b98b/0x1b990 (local
  banks), and the 113xxx AUTO-FOOTPRINT floor/walls 0x1b97c/0x1b972/0x1b977 (retail inlines them as a
  grid-rectangle loop == emit_tile span 0/0).  **GROUND TRUTH:** a retail capture of the decode emit sequence
  (hooks on 0x58c910/0x58ca80 across the cutscene chain, `runs/room-render-gt`, `tools/flow/map_decode_fields.json`)
  cross-referenced with the cell (tile id, shape) histograms (`map_data.py --cells`).  A host probe decodes
  the real DATA 1023/1025 with recording emit-stubs — every emit_tile/emit_obj (bank, slot, flag, count)
  matches retail EXACTLY (house 111 tile/50 obj; errands 98 obj + 37 captured-arm tiles + 78 direct-write
  113xxx tiles = the cell histogram).  RESOLVED the architecture Q: **the room swap DOES re-run 0x587e00 per
  room** (3 decodes captured: town@flip1434/house@3671/errands@4290) and **param_3 (local_918) = 0x14 for
  every town-area room** (normalizes to 0 = the 113xxx tile frame).  Deferred: PORT-DEBT(decode-occlusion-mark)
  (the 113xxx shape-1/2 region-B/D culling marks, ~7 errands cells, no visible tile).
- **M3 — room-keyed backdrop reload + per-room camera.**  `reload_room_backdrop(key)` (town_render_free +
  load_room + camera snap + map-bounds) fires when `cutscene_room_key` changes (arrival→house) and on chain
  COMPLETE (→ errands).  The house/errands SETTLED camera origins are HARNESS-CAPTURED
  (`tools/flow/room_camera_fields.json` reading the scene view `*(*(0x8a9b50)+0x104c)` cur_x/cur_y across the
  chain — both STATIC): house (89600,3200) map 153600x51200, errands (0,16000) map 108800x64000.  The town
  cast/effects (spawned for DATA 1022) are suppressed for non-town rooms (`room_is_town`) so they don't
  render over the new backdrop.  Retires PORT-DEBT(cutscene-room-render).
- **Tooling — `--no-frame-limit`** uncaps the in-game 60 FPS gate (gated on g_game_active; the title/menu nav
  stays capped — uncapping the WHOLE run desyncs the frame-keyed title nav, newgame slid flip 750→5403) so
  the full ~13000-frame cutscene→errands replay captures in ~9 s.  Verified: game_enter@1116, chain
  COMPLETE@hold 11365 (identical to the capped boot).
- **M4 — the errands-render CRASH fixed.**  Rendering DATA 1025 access-violated (0xC0000005): an under-loaded
  errands tileset bank (PORT-DEBT(assetreg-clone-defer)) made `ar_sprite_slot_frame`'s unbounded
  `frames[frame_id]` read OOB → a garbage cel `game_present_blit` deref'd.  FIX: bound frame_id against
  `slot->f_38` (the `ar_sprite_slice` frame count) when known (>0) — under-loaded bank → NULL (tile culled,
  a gap) not a crash; f_38==0 keeps the retail-faithful raw read.  Faithful: retail's f_38 ≥ every tile's
  frame index (no behavior change there).  RESULT: the full chain runs to completion (exit 0) + the errands
  backdrop RENDERS (the 113xxx floor/walls + the multi-level town scene at camera (0,16000)).

**VERIFIED visually (on the feed):** `pf7485.png` = the HOUSE facade (timber frame/roof/door/fence,
parallax sky), `errands.png` = the ERRANDS multi-level town scene (buildings/staircase/chimney/hedges/grass).
Both render coherently with the town cast suppressed.  Captured `C:\oss-osr\port-rooms.osr` (12998 frames,
town→house→errands).

**OPEN (USER) / NEXT MOVE.**  (1) **The FREEROAM HAND-OFF** (the main goal's last mile): at the errands room
stop the cutscene sequencer + run `character_step` on live `g_game_drive.input.axis_held` (the `+0x200==0`
char-AI path — quirk #103; the mover is DONE bit-exact) so Arche is controllable.  (2) **Load the
under-loaded errands tileset banks** (the render GAPS — the f_38 guard culls frames those banks lack;
PORT-DEBT(assetreg-clone-defer) now exercised by errands).  (3) **The room CAST** (house/errands NPCs;
Phase 2b — the town cast is suppressed, not yet replaced).  (4) **A tick-aligned port↔retail studio diff**
needs a matched-cadence nav: the port reaches the rooms via the cutscene-verify nav at very different ticks
than retail's control-path nav (house port-tick ~3185 vs retail ~1103), so the automated tick-join
misaligns — a single-file `osr_view.exe 'C:\oss-osr\port-rooms.osr'` scrub + the feed images are the current
verification.  The decode is bit-exact (the rigorous gate).  Plan: `plans/controllable-arche-faithful.md`.

**Module layout (this ckpt):** `src/game_world.{c,h}` (+`game_world_room_render_cfg`), `src/town_render.{c,h}`
(+parallax params, +`map_decode_cfg`), `src/map_decode.{c,h}` (the cfg + the 14 new arms + the 2 block
helpers), `src/main.c` (`load_room`/`reload_room_backdrop`/`room_camera_origin`, `--no-frame-limit`, the
room-swap trigger + actor suppression), `src/asset_register.c` (the `ar_sprite_slot_frame` f_38 guard).
Tools: `tools/flow/map_decode_fields.json` (the decode emit-capture spec), `tools/flow/room_camera_fields.json`
(the per-room camera spec).  Artifacts (gitignored): `runs/room-render-gt/{emit,camera}` (the retail
captures), `C:\oss-osr\port-rooms.osr` + `errands.png`/`pf7485.png`.  Tests: +5 (game_world_room_render_cfg,
map_decode_cfg_init, map_decode_house_arms, map_decode_block_arms, map_decode_errands_arms,
ar_sprite_slot_frame_f38_bound).

**OPEN RE threads (don't block):** the errands questline `0x4dc510` (the freeroam gameplay); the later
`+0x200=1` transfer; the A/B portrait facing (dynamic).

## Where we were — ckpt 129

**TRACE STUDIO v2 — M6 COMPLETE: the TICK-JOIN STUDIO.  Both sides' `.osr` are now paired by the
deterministic `sim_tick` (the openrecet E3 identity join — group each side by tick, take the LAST flip per
tick = the presented state, join on the tick union, keep honest port-only/retail-only GAPS — NOT a
flip-axis ±drift search), and `osr_view` grows a native PORT|RETAIL|DIFF three-panel + a precomputed diff
heat ribbon.  This is the first usable v2 deliverable: a frame-by-frame 1:1 port-vs-retail scrub on the
parity (sim_tick) axis.**  1002 host pass (unchanged — tooling only, no `src/` engine change).  Two commits:
`2788ed9` (M6a) + `57260be` (M6b).  Tick-join montages on the feed.

**M6a — the JOIN verdict + the streaming reader (`tools/trace_studio2/`, runnable from WSL):**
- `osr.py` grew `stream_records(path, types)` / `stream_frames` / `read_header` — a BLOCK-BUFFERED iterator
  (mirrors the C `build_index` scan) that yields only the requested small records and SKIPS the bulky
  BLIT/TEXT/SHEET/SNAP payloads by seek/pointer arithmetic (never materializes them).  Streams the full
  **1.9 GB `retail.osr` (37673 frames) in ~11 s with NO OOM** — RETIRES the survey-flagged `parse()` OOM
  debt (and the `run_proxy.sh` 256 MB SUMMARY band-aid can be lifted when convenient).  Byte-identical to
  `parse()` on the small port file.  `parse()` (whole-file) stays for small captures + the round-trip tests.
- `pair.py` (NEW): streams both sides → `load_side` builds per-frame `(ordinal, flip, tick)` + the
  last-flip-per-tick map + the anchors; `join` walks the sorted tick union → paired / port-only / retail-only.
  `report` prints the paired count, honest gaps, per-shared-anchor RNG assertions (the join-validity proof),
  and the flip-axis drift contrast (`naive_flip_drift` = retail_ge_flip − port_ge_flip).  `--write-pairs`
  emits `pairs.json` (a reference/inspection artifact — `osr_view` recomputes the join natively, so there's
  NO stale-intermediate dependency).  Arg-order tolerant (side 0/1 auto-sorted).
- **VERDICT on (port-m5.osr, retail-snap.osr): PASS** — 190 tick-paired, 2 honest port-only gaps (ticks
  41/91 = retail coalesced those ticks, quirk #99), 12403 retail-only gaps (the port smoke capture only
  reached tick 191; retail ran to 12607), all 3 shared anchors RNG-aligned (newgame `0x404a0a8f`,
  prologue/game_enter `0x40d00581`).  game_enter lands at port flip 1116 vs retail 1242 → naive same-flip
  pairing would silently drift +126 flips (~63 ticks); the tick-join is immune.

**M6b — the native three-panel (`tools/osr_view`):**
- `osr_view.exe <port.osr> <retail.osr>` (two args) → `run_dual`; one arg → `run_single` (the original,
  refactored, unchanged behaviour).  Branch in `main`.
- Opens TWO `osr_scrub` sessions (verified safe: `zdd_create` is a clean per-instance alloc with no
  singleton; the only file-global `g_present_hwnd` is used solely in the window-present path that offscreen
  recon never touches — recon renders to a system-memory dest).  Two DDraw devices + the DX11 swapchain
  coexist with no contention.
- `build_join(port, retail)` builds `tick_index` per side (frames are in flip order so the std::map
  last-write-wins = the last flip per tick) → a `JoinEntry` vector over the sorted tick union (same semantics
  as `pair.py`, NO `pairs.json` dependency — the viewer is self-sufficient).
- `diff_image(a,b,out,…)` = per-pixel cross-side diff → an amplified DIFF panel (faint port-luma silhouette
  where equal, yellow→red ramp by magnitude where divergent) + `(differ_px, maxd)`.
- The diff HEAT RIBBON: per-paired-tick `differ_px` precomputed in the BACKGROUND (12 real renders/UI-frame,
  gaps zero instantly; finishes in ~16 frames for this pair), drawn as an aggregate worst-per-column strip
  (green=pixel-exact, yellow→red=divergence by log-scaled differ_px, blue=honest gap, dark=computing) with
  click-to-seek; nav buttons `|< first paired` / `worst diff >|` / `next diff >`.  A `Panel` struct
  generalizes the DX11 frame texture to N panels.  Panels auto-scale to fit the window width.
- Makefile: link `../../src/osr_emit.c` into ALL 3 osr_view targets (osr_view/gdi/prof) — `zdd.c` hard-refs
  its M5 sink taps (`osr_emit_blit/evict/clear/dc_open/dc_close`); `osr_emit.c` self-gates on `g_oe_fp==NULL`
  so it's a no-op in the viewer, only the symbols.  **Fixes a latent M5 link break** (osr_view/prof hadn't
  been rebuilt since the M5 taps landed in zdd.c).

**VERIFIED HEADLESS (no GUI needed — `osr_prof.exe dump <idx> <bmp>` uses a message-only window, runs via
WSLInterop, and shares the exact `osr_scrub` engine the GUI wraps):**
- The join frame indices match `pair.py` EXACTLY: port idx 1115 → flip 1116 tick 0, retail idx 1241 → flip
  1242 tick 0 (so `osr_scrub`'s ordinal == `pair.py`'s ordinal — the Python verdict and the C renderer agree).
- Two-session reconstruction works; the cross-side diff is real and meaningful: **sim_tick 0 reconstructs
  `differ_px==0` — PIXEL-IDENTICAL port vs retail.**  CORRECTION (the draw inspector caught it): tick 0 is
  the game_enter TRANSITION frame, NOT the town establishing shot — its full render is near-black (53
  non-black px) because a LATE mid-frame scene-transition CLEAR (quirk #105) wipes a fully-composed town
  within that frame (visible at the inspector's K=894, gone by K=1789); both sides do it identically (hence
  differ_px==0).  The settled town renders a few ticks in (e.g. tick 97 `differ_px=264`, 0.09% — the
  animated butterflies, a small localized divergence the studio surfaces).  Montages on the feed.
- **USER-CONFIRMED the GUI** ("studio looks good"; off-screen tile-load regions are wanted; the post-tick-191
  black PORT panel is the honest gap — port-m5 ends at tick 191).

**M7 — the DRILL-IN + the NOTE hand-off (this ckpt, USER-requested "the M7 stuff plus a simple note
system").**  Commits `953ee74` (notes) + `b568104` (inspector engine) + `6279274` (inspector GUI).

- **The NOTE / mark system (the human→agent contract — openrecet N4, our v1 marks/worklist returning).**
  In osr_view dual mode the USER drags a crop rect on any panel (frame-space, drawn live on all 3) + types a
  note → it persists to `osr_notes.jsonl` beside the `.osr` (one JSON line: tick, port_flip, retail_flip,
  crop[x,y,w,h], differ, text); a notes-list panel seeks (`go`) / deletes (`x`); notes load on startup.
  Keyboard scrub is suppressed while typing (`WantCaptureKeyboard`).  Gap panels now LABEL "no frame at this
  tick" instead of bare black.  `tools/trace_studio2/notes.py` is the agent READ side: reads the JSONL,
  resolves each note's tick → per-side frame index via the same join as `pair.py`, and `--render`
  reconstructs the cropped port|retail|diff at that tick (`osr_prof` dump + PIL crop), `--feed` pushes it.
  So a mark says exactly "look HERE at THIS sim_tick" and round-trips to a precise visual.  VERIFIED
  end-to-end headless: a hand-crafted JSONL in the EXACT C `note_write_line` format parses; notes.py
  resolved + rendered the crops (tick-97 crop differ 264, tick-0 whole-frame differ 0).
- **The DRAW INSPECTOR (openrecet N3 — the self-serve "which draw made this pixel" loop).**  `osr_scrub`
  grew: `frame_ndraws`/`frame_draws` (the ordered BLIT/TEXT/CLEAR list + human labels like "onto res=1002
  f=0 @(0,0) 80x352"), `render_rgba_upto(idx,K)` (reconstruct applying only the first K draws — watch a
  frame build; replay_frame refactored to `replay_frame_upto`, readback factored to `readback_rgba`),
  `pick_draw(idx,px,py)` (which draw last changed a pixel — one incremental pass: clear → apply each draw →
  sample the RGB565 pixel, releasing the GDI DC each step so text flushes before sampling), `resolve_nonempty`
  shared by all.  GUI (UNIFIED into the main 3 panels per the USER's openrecet-UX ask — NO separate window):
  a **frame / draw-drill MODE toggle** + per-panel **show** checkboxes (focus one side / hide the diff); in
  drill mode ONE "draw K (both sides)" slider drives `render_rgba_upto` on BOTH sides synchronously, so the
  DIFF panel shows exactly where the two draw sequences diverge.  A focus radio (port/retail) picks the draw
  list + pick side; the selected draw's rect is highlighted green on every panel.  Interaction split: a panel
  DRAG = a crop (notes, any mode), a CLICK in drill = pixel→draw pick on that side (the InvisibleButton owns
  both → no window-move, the ckpt-129 bug fix).  ENGINE VERIFIED headless: `render_rgba_upto(all)==render_rgba`
  (differ 0); on port frame 1309 the build-up is clean (K=1 CLEAR=black → K=308 full town → K=616 +banner),
  `pick(200,150)`=draw #615 the banner; the cross-side up-to-K diff at tick 112 peaks ~1020px @K≈158 then
  settles to 380 — it reveals sequence divergence.  GUI interaction = USER visual-verify.
- **The DRAW-SEQUENCE-MATCH direction (USER, for max faithfulness):** the drill exists to eventually MATCH
  the port↔retail draw SEQUENCE, not just the final pixels.  Already a concrete lead: at tick 112 port emits
  616 draws vs retail 634 (18 extra retail draws) — a structural mismatch to reconcile.  This is the parity
  bar for the room-render/freeroam port: diff the draw streams (the `render_diff` lens, openrecet survey #6)
  and align them.

**M8 — the GAME-STATE panel (this ckpt, USER-requested; openrecet orv3_state model).**  Commits `ba0b801`
(M8a) + `8da3dcb` (M8c) + `2a6f424` (M8b).  A native, OPT-IN engine-state pillar in the studio.
- **Format (M8a):** `OSR_STATE` (=14) in `src/osr_format.h` — a generic list of NAMED scalar fields
  `{name[16], kind(hex/int/f32), i64 ival, f64 fval}` (`osr_enc_state`/`osr_dec_state_field` + u64/f64
  helpers).  Extensible: the emitter appends what it reads, the decoder is field-agnostic.
- **Port emit (M8a):** `osr_emit.c` per-frame accumulator (`osr_emit_state_field` push +
  `osr_emit_state_enable`) flushed as one OSR_STATE after each FRAMEBEG; armed by `--osr-state`.  `main.c`
  pushes `rng` (`rng_peek_state` = `DAT_008a4f94`) + `rngcalls` (new `rng_call_count` in `rng.c`) at the
  drive_present flip site — **ADD MORE `osr_emit_state_field` calls there as you annotate** (player px/py,
  scene id, flags, dialogue state).  `osr.py` decodes (`STATES` dump, streamed; SUMMARY line).
- **Retail emit (M8b):** opt-in `OSS_OSR_STATE` (`proxy_config.h`); the proxy flip hook (`engine_hooks.h`
  `eh_flip_cb`) writes one OSR_STATE with `rng` (eh_read_seed, a free global read) after FRAMEBEG.  Retail
  `rngcalls` is `PORT-DEBT(osr-state-rngcalls-retail)` — needs a 0x5bf505 trampoline counter.  **Add the
  matching read here when you add a port field.**
- **Viewer (M8c):** `osr_scrub_frame_state(idx)` decodes a frame's OSR_STATE; an "ENGINE STATE" table
  (field | port | retail) shows the union of both sides' fields, formatted by kind, port≠retail red.
- **RNG census FOLDED IN:** the live per-tick rng/rngcalls diff is the panel's first content — it
  SUPERSEDES `tools/rng_tick_diff.py` (archived → `tools/archive/`, banner'd).  `rng_consumer_census.py`
  (which fn draws the LCG — ATTRIBUTION) is a different concern, KEPT.
- VERIFIED headless: a 150-frame port capture emits STATE=150 (rng=0x4f5347 pinned, rngcalls=0);
  `osr.py STATES`/`SUMMARY` + `osr_scrub_frame_state` read it; host test `test_osr_emit_state` (+1, 1003).
  **OPEN (USER): the GUI panel + the retail `OSS_OSR_STATE` capture.**  CLAUDE.md carries the how-to.

**OPEN (USER): GUI visual-verify of the M7 additions** — the windowed DX11 app can't be driven from WSL.
In `build/osr_view.exe 'C:\oss-osr\port-m5.osr' 'C:\oss-osr\retail-snap.osr'`: (1) drag a crop on a panel,
type a note, Add → it lands in the list + `C:\oss-osr\osr_notes.jsonl` (then I run `notes.py --render`);
(2) flip to **draw-drill** mode → slide K to watch both sides build, the DIFF lights up where they diverge;
toggle the show-panel checkboxes; click a panel pixel to pick its draw.  **USER-CONFIRMED so far:** the
3-panel scrub + the note round-trip (cropped Sana = pixel-identical); window-drag-while-cropping was the one
bug, FIXED.

**NEXT MOVE.**  The studio is usable + self-diagnosing → the PAUSED room-render/freeroam arc is UNBLOCKED
(the whole reason v2 was pulled forward, ckpt 125): RESUME it — render the house/errands ROOMS
(`plans/controllable-arche-faithful.md` Phase 2a; scene ids house=1023/errands=1025) → the freeroam hand-off,
verified frame-by-frame in the studio AND draw-sequence-matched (the 616-vs-634 kind of mismatch reconciled).
**The standing WORKFLOW (CLAUDE.md, USER-set):** inspect every render divergence in the draw drill; GIVE the
`osr_view.exe <port> <retail>` command on any visually-confirmable change; the USER drops crop marks →
`notes.py --render`; a studio shortcoming is a new studio FEATURE.  Remaining studio polish (openrecet survey
4/5/6: `.osr` slice tool, capture cache + one orchestrator command, the draw-program semantic panel) is
pull-when-needed.  **CAVEAT for a real comparison:** the port-m5
capture only reaches tick 191 (its intro-1 nav is short); a matched-length port capture awaits the freeroam
port, so for now the 190-paired region is the working demo.  Roadmap: `plans/trace-studio-v2.md`
§openrecet-v3-survey (items 1/2/3/7 DONE this ckpt; 4/5/6 remain).

**Module layout (v2, current additions):** `tools/trace_studio2/osr.py` (+ streaming iterator),
`tools/trace_studio2/pair.py` (the tick-join verdict), `tools/trace_studio2/notes.py` (the note read side),
`tools/osr_view/osr_scrub.{c,h}` (+ frame_draws / render_rgba_upto / pick_draw), `tools/osr_view/
osr_view_imgui.cpp` (dual `run_dual` / `build_join` / `diff_image` / ribbon / notes / draw inspector;
`run_single` unchanged), `tools/osr_view/Makefile` (+ osr_emit.c).  Capture/recon modules unchanged from
ckpt 128 (see below).  Artifacts on `C:\oss-osr\`: `port-m5.osr` + `retail-snap.osr` (the two sides),
`osr_notes.jsonl` (the marks), `note_render/` (notes.py crops), `cmp_*.png`/`drawbuildup2.png` (montages).

**OPEN RE threads (don't block):** none new for v2.  The PAUSED movement-system arc (carried): the
errands/house ROOM-render path → the freeroam hand-off; the A/B portrait FACING (dynamic); the errands
questline `0x4dc510`; the later `+0x200=1` transfer.

## Where we were — ckpt 128

**TRACE STUDIO v2 — M1 through M5 COMPLETE.  M5 (this ckpt): the PORT `.osr` EMITTER — the port writes
the SAME draw stream the retail proxy captures, from its own sinks, and `--osr-replay` of the port's OWN
`.osr` rebuilds its frames `differ_px==0`** (newgame menu flip 700 / prologue 900 / town 1250 vs the
port's live `--capture-frames` BMPs — the port stream is fully SELF-CONTAINED).  Both sides now produce
one codec's files: retail via the native Frida-free proxy (`tools/capture_proxy/`, M1–M4 — BLIT/SHEET/
TEXT/FONT/BLEND/CLEAR/SNAP, 71/71 `--validate` snaps clean, ckpt 127), the port via `src/osr_emit.{c,h}`
(M5, commit `cc99f3a`).  Reconstruction: `--osr-replay` (`src/osr_replay.{c,h}` → `src/osr_recon.c`) +
`tools/osr_view` (native ImGui scrubber, ~1.4 ms/frame) — both open either side's file unchanged.
1002 host pass (+2).  Design + build order M1→M7: `docs/plans/trace-studio-v2.md`.  **v1 is RETIRED
(ckpt 128, USER directive): do not start the `:8779` web serve or generate `tools/trace_studio.py`
captures — v2 (.osr + osr_view) supersedes it; the marks/worklist hand-off returns inside osr_view at
M7.  Old `runs/trace-studio/` sessions stay read-only (their navs are the proven scenario inputs).**

**M5 detail (durable):**
- `src/osr_emit.{c,h}` — pure C, host-linkable, every sink gating internally on `--osr-emit` (the
  call_trace discipline).  Mirrors the proxy hook map 1:1:
  - FRAMEBEG/PRESENT at `drive_present` — present-then-framebeg, the proxy's exact frame structure
    (draws under FRAMEBEG(f) are issued after flip f, presented by flip f+1; immaterial to the
    sim_tick join).  FRAMEBEG carries `g_present_frame` + `g_sim_tick_count`.
  - BLIT at the 5 zdd primitives: `zdd_emit_blit` (zdd.c) extended with dest/desc/srcw+srch; the
    emitter reads ow/oh/ox/oy/state off the src cel + res/frame from render_id.
  - SHEET: per-CEL surface grab at first blit, through an INJECTED surface reader (main.c registers a
    `zdd_object_lock` + `get_locked_info` + new `zdd_object_get_locked_width` reader; the host test
    injects canned pixels — that's what keeps osr_emit.c pure).  Hash mirrors sheet_grab.h EXACTLY
    (FNV-1a over w:u32, h:u32, bitcount:u16, then pitch*h bytes); tombstoned eviction at
    `zdd_object_dtor` (the ckpt-126 stale-sheet lesson) + an emitted-dhash set so identical re-grabs
    don't re-emit.
  - CLEAR at `zdd_object_clear` (an ORDERED draw, shared seq — quirk #105); BLEND for mode-4 alpha
    (blend_grab.h's exact per-mode LUT sizing, content-dedup'd, streamed without a scratch concat);
    TEXT via a per-HDC shadow bound at `zdd_object_get_dc`/`release_dc` (mirrors in the
    glyph_render_win32 ops + main.c dialogue rows; the banner-cel compose is correctly FILTERED — its
    pixels reach the file via the composed cel's SHEET); FONT at the `ar_gdi_create_font` chokepoint;
    ANCHOR at `emit_anchor`; SEED at both rng_srand pin sites (boot + enter_game).
  - BLIT/CLEAR/TEXT emit only when the dest is the PRIMARY compose surface (dst_handle 1) — retail's
    observed stream shape (dst 100% backbuffer); offscreen composition reaches the file via SHEET
    content at first primary blit, the same accepted staleness window as the proxy.
- **Live proof (intro-1 nav, 1500 flips):** 316k blits / 173 sheets (0 grab fails) / 18k texts / 11
  fonts / 38 blends / 909 clears; `osr.py` reads 100% named + 100% dhash/dst coverage (above retail's
  89-90%), all 4 anchors at the proven flips (game_enter@1116), both seed pins.  Self-reconstruction
  `differ_px==0` on all 3 checked flips (capture at flip f pairs with recon frame f-1 — the label
  offset).  Port dhash ≠ retail dhash stays expected (Lock pitch is inside the hashed bytes) —
  `PORT-DEBT(osr-sheet-dhash-xside)`; `(res,frame)` is the cross-side join.
- Host tests: `tests/test_osr_emit.c` (+2) — inactive-noop safety + a full write→`osr_replay`
  read-back round trip (frame structure, shared seq, sheet dedup + post-evict no-reemit, dest filter,
  font/blend refs, anchor/seed).

**The thesis (USER-directed):** capture the DRAW-CALL STREAM (DDraw blits + GDI text + state + dedup'd
source surfaces) via a proxy `ddraw.dll` — NOT pixels — and reconstruct frames 1:1 on Windows (real GDI →
text is bit-exact; the offline-GDI blocker is void). Join the two sides by deterministic `sim_tick`, not a
pixel-drift search. Everything heavy runs Windows-side; WSL only orchestrates + reviews via `/mnt/c`.

**What landed (M1+M2+M3a+M3b+M3c):**
- **M3c** — the SOURCE pixels + surface identity. The "risky COM vtable wrap" was UNNECESSARY: the blit
  decompiles (`docs/decompiled/by-address/5b9a40.c` etc.) showed each cel/dest holds a real
  `IDirectDrawSurface7*` at `+0x2c`, and the engine calls `dest->Blt(&dr, src, &sr, flags, NULL)` via vtable
  +0x14 — so the proxy interns those RAW surface pointers + grabs source pixels straight from the blit
  detour, NO surface wrapping + NO per-blit COM overhead. `src/osr_format.h` grew the variable-length
  `OSR_SHEET` record (24-B prefix `dhash/res/frame/w/h/pitch/pixfmt/codec/byte_len` + raw pixels) +
  `osr_enc_sheet_prefix` (streams big payloads into the ring with ONE copy) + a host round-trip test.
  `tools/capture_proxy/surface_id.h` (NEW, ptr→stable-handle intern), `sheet_grab.h` (NEW — Lock READONLY on
  first sighting, FNV-1a mirroring `asset_register.c`'s w/h/bitcount+pixels seed order, one dedup'd SHEET per
  surface ptr, dhash cached), `engine_pixfmt.h` (NEW — `DDPIXELFORMAT`→`OSR_PIXFMT_*`). `engine_hooks.h`
  reads the dest surface (`*(void**)(arg0+0x2c)`) → `dst_handle` (+ the first one re-stamps the header
  pixfmt/screen from its `DDSURFACEDESC2`) and the src surface (`*(void**)(cel+0x2c)`) → SHEET → BLIT
  `dhash`. `osr_writer.h` re-stamps the header at offset 0 from the bg thread (`osr_w_fixup_header`, the only
  non-sequential write). `osr.py` decodes SHEET (+`SHEETS` dump + dst/dhash coverage in SUMMARY). **PROVEN**
  (`m3c-verify` nav→town, `C:\oss-osr\retail.osr`): header re-stamped `640×480 RGB565` (was UNKNOWN),
  `dst_handle` 100% set / 1 distinct (= the backbuffer, correct for a 2D compositor), src `dhash` 100% set,
  **496 SHEETs / 420 distinct dhash / 9.4 MiB raw RGB565**, all 3 anchors + both seed pins byte-identical to
  M3b, 90% named, **912 fps** (<4% over M3b's 950 — well under the 10% budget; the Lock/grab is once-per-
  surface, no stall/crash). Captured sheets are coherent: 640×480 backdrop/scroll layers, a sparkle particle
  series (22→20→18→14→12→10→8→6), 18× 160×176 portrait busts, 32×32 town tiles, tall 80×N sprite columns.
  FOLLOW-UPS (tagged PORT-DEBT, NON-blocking M3d/M4): `osr-sheet-compression` (raw pixels, miniz deferred),
  `osr-sheet-dhash-xside` (retail dhash won't byte-match the port's cross-side — native pitch/pixfmt differ →
  a legit render_diff `[decode]` signal; `(res,frame)` stays the primary join), `osr-alpha-src-grab` (mode-4
  alpha is a GDI/`paint_ctx` blend, its `+0x2c` source grab best-effort).
- **M3b** — the BLIT op stream. `src/osr_format.h` grew the `OSR_BLIT` record (fixed 76-B payload =
  `tools/render_diff.py`'s schema: `va/seq`, `res/frame`, `dhash/dst_handle`, `dx,dy,reqw,reqh,sx,sy`,
  `ow,oh,ox,oy`, `state/ckey/bmode/mode`) + host-tested enc/dec. `tools/capture_proxy/render_id.h` (NEW) =
  the retail mirror of `src/render_id.c`: a cel→`(res,frame)` open-addressing registry, populated by the
  resolver detour `0x418470` which COMPUTES the cel AT onEnter from the decompile
  (`cel = *(*(*slot) + (frame&0xffff)*4)` whenever the bank is decoded, i.e. `*(int*)*in_ECX != 0`) — so
  M3b stays on the proven onEnter-only `va_detour.h` framework, NO onLeave. (The undecoded-bank first
  resolve is skipped and self-heals on the cel's next resolve; banks decode early + cels persist, so steady
  state is 89% named.) `engine_hooks.h` detours the 5 blit VAs (`0x5b9a40` onto / `b70` keyed / `ae0` rects
  / `bf0` clipped / `0x5bd550` alpha), reading geometry + cel-fields per the decompiled arg conventions
  (thiscall ECX=cel for 4; alpha is cdecl with cel=arg[1], dx=arg2..sy=arg7, ckey=arg8, bmode from the
  forwarded blend-desc `this` in ECX +0x0). The flip hook is restructured to FRAMEBEG-at-open / draws /
  PRESENT-at-flip with a per-frame draw `seq` (reset each FRAMEBEG). `osr.py` decodes BLIT + adds the `BLITS`
  draw-list dump + per-VA/named% SUMMARY stats. **PROVEN** (nav `runs/proxy-m2b` → game_enter): 867k blits /
  2377 frames, 89% render-id-named, all 3 anchors at the M2b flips + both seed pins; the town establishing
  shot (flip 1250, 1815 blits) decodes coherently (res=1002 backdrop columns 8×80px, res=2234 `clipped`
  32×32 sub-tiles at the camera scroll dst=-32, KEYSRC st=0x8000 ckey=0xf81f) — matching the documented town
  render. **PERF — FULL TURBO RESTORED (measured):** in-game town ~25 fps first cut → ~56 fps (RWX pages,
  `ee55e5b`) → **~950 fps (E9-jmp trampoline, `50ec26b`)**. The 6 hot hooks (resolver + 5 blits) are now
  inline 5-byte E9 jumps with ZERO exceptions/hit (`trampoline.h`); a 30 s run captures 29k frames / 14.6M
  blits, geometry byte-identical to the INT3 baseline. `dhash`/`dst_handle` stay 0 retail-side until M3c.
- **M3a** — the `.osr` writer. `src/osr_format.h` = the shared pure-C codec (64-B header + framed
  `{type,len,payload}` records, little-endian, append-only + truncated-tail recoverable for the harness
  hard-kill) used by the proxy, the port emitter (M5), the reconstructor (M4), and `osr.py`.
  `tools/capture_proxy/osr_writer.h` = a double-buffer ring drained by a bg thread to a `C:\` `.osr`,
  fflush per drain (durable to the last drain on a hard kill); the engine thread (inside the VEH callbacks)
  only locks+memcpy, so disk latency never stalls it. Records wired: FRAMEBEG+PRESENT per flip (the
  tick-join axis), ANCHOR at the anchors, SEED at the title pin + per-map re-pin. `tools/trace_studio2/
  osr.py` = the reader/validator. 6 host tests (988 pass). **PROVEN on a real boot:** retail.osr (417 KB,
  11585 frames flip 1..11585 / sim_tick 0..10358), all 3 anchors at the exact M2b flips, both seed pins, no
  torn tail; ~800 fps WITH capture (no regression — the hot path logs calls, not pixels). Config:
  `OSS_OSR`/`OSS_OSR_PATH`/`OSS_SCENARIO`.
- **M1** — the retail exe imports ONE DDRAW symbol (`DirectDrawCreateEx`) with a FIXED base 0x400000 +
  relocations stripped, so a proxy `ddraw.dll` next to the exe auto-loads (no Frida, no injector).
  `ddraw_proxy.c` forwards to the real SysWOW64 ddraw.
- **M2a** — IAT-patched turbo/lockstep clock (`clock.h`, port of the agent's `installTurboHooks`), env
  config (`proxy_config.h`), harness thread (`harness.h` — hide window, `WM_ACTIVATEAPP` keep-alive,
  auto-dismiss the launcher dialog). Headless turbo boot in ~1 s.
- **M2b** — the engine-VA detour layer. `va_detour.h`: INT3 + a vectored exception handler (NO
  length-disassembler, no vendored lib) — patch byte→0xCC, VEH runs the onEnter callback off the `CONTEXT`,
  then restore/rewind-Eip/trap-flag/single-step/re-arm resumes the real op in place; one-shot disarm
  supported. `engine_hooks.h`: flip `0x5b8fc0` (+lockstep advance + throughput heartbeat), sim-tick
  `0x43d1d0`, one-shot title seed-pin `0x56c070`→`DAT_008a4f94`, newgame/prologue/game_enter anchors,
  per-map RNG re-pin `0x41f200`. `engine_input.h`: ring injection (hook `0x43c110`, fill the input ring;
  manager re-read each poll for sub-menus; ts from `[esp+4]`). PROVEN: newgame@flip652 → prologue@1000 →
  game_enter@1242 (RNG re-pin fires) → sim_tick ~1:1 with flips.

**Module layout (current, all under `tools/capture_proxy/` unless noted):** `ddraw_proxy.c` (entry/forward
+ init order), `proxy_log.h`, `proxy_config.h` (incl. `OSS_OSR_SNAP_EVERY`/`OSS_OSR_SNAP_FLIPS`),
`iat_hook.h`, `clock.h`, `va_detour.h`, `engine_hooks.h` (anchors/blits/dtor-evict/CLEAR `0x5b9410`/SNAP
grab), `engine_input.h`, `engine_gdi.h` (M3d TEXT/FONT), `render_id.h`, `trampoline.h`,
`surface_id.h`+`sheet_grab.h`+`engine_pixfmt.h`, `blend_grab.h` (M4-alpha), `harness.h`, `Makefile`,
`ddraw_proxy.def`, `run_proxy.sh`.  Port-side: `src/osr_format.h` (the shared codec: BLIT 88-B, SHEET,
BLEND, FONT, TEXT, CLEAR, SNAP), `src/osr_replay.{c,h}` (streaming reader), `src/osr_recon.c` (the
`--osr-replay` reconstructor + the SNAP `differ_px` validate), `src/recon_apply.{c,h}` (shared sinks),
`src/osr_emit.{c,h}` (M5 — the port emitter; sinks tapped in `zdd.c`, `glyph_render_win32.c`,
`asset_register_win32.c`, `main.c`), `tools/osr_view/` (ImGui scrubber + `build/osr_prof.exe`
frame-dumper).  `tools/trace_studio2/osr.py` = the Python reader.  Artifacts on native NTFS
`C:\oss-osr\` (`retail.osr` = the ckpt-126 USER-confirmed capture; `retail-snap.osr` = the ckpt-127
snap+clear capture; `port-m5.osr` + `m5caps/`+`m5recon/` = the ckpt-128 M5 smoke).  Nav:
`runs/proxy-m2b/nav.jsonl` (proxy) / `runs/trace-studio/intro-1/edit.trace.port.jsonl` (port).

**NEXT MOVE — M6: the tick-join studio.** Pair both sides' `.osr` by `sim_tick` (the identity JOIN,
openrecet E3 — compute the pairing ONCE, render honest gaps for port-only/retail-only spans, NO pixel
drift search), then grow osr_view: port|retail|diff three-panel + a precomputed per-pair diff ribbon
(seek-to-worst-frame), per the roadmap order in `plans/trace-studio-v2.md` §openrecet-v3-survey (items
2→3).  Both sides' files already open in osr_view/recon unchanged (one shared codec), so M6 is pairing +
viewer UX, not new capture work.  A fresh retail capture for the pairing already exists
(`C:\oss-osr\retail-snap.osr`); the port side is one `--osr-emit` run per scenario.  The held-axis leaf
inject (`0x5ba520`) stays DEFERRED until freeroam capture needs it.  PORT-DEBT follow-ups
(`osr-sheet-compression`, `osr-sheet-dhash-xside`, `osr-alpha-src-grab`) remain NON-blocking.

**M3c validation detail (durable):** the surface convention was RE'd from the blit decompiles
(`docs/decompiled/by-address/5b9a40.c`/`5b9ae0.c`/`5b9b70.c`/`5b9bf0.c`/`5bd550.c`): every primitive does
`*(dest+0x2c)->Blt(&destrect, *(cel+0x2c), &srcrect, *(cel+0xd4)|0x1000000, 0)` via vtable +0x14 (= Blt on
the IDirectDrawSurface7 layout). So `cel+0x2c` = the SOURCE surface (null-guarded — the engine's `if
(*(cel+0x2c)==0) return 1` early-out), `dest_arg+0x2c` = the DEST surface, both real ddraw surfaces.
`dest_arg` = stack arg0 uniformly (incl. the cdecl alpha `0x5bd550`). The clipped primitive `0x5b9bf0`
computes a clipped src rect from `cel+0xc/0x10/0x14/0x18` (clip origin+extent), but the BLIT record stores
the RAW pre-clip geometry (reconstruction redoes the clip).

**M3b validation detail (durable):** the 5 blit VAs' arg conventions were RE'd from the engine decompiles
(`docs/decompiled/by-address/5b9a40.c` etc.) cross-checked against the port's `zdd_emit_blit` field set —
NOT from `retail_fields.json` (the blit entries there were throwaway; the current file has only the flip
spec). onto/keyed: `(dest, x, y)`, dst extent = cel +0xb8/+0xbc, sx/sy=0. rects: `(dest, dx, dy, dw, dh,
sx, sy, sw, sh)`. clipped: `(dest, dx, dy, w, h, sx, sy)` RAW pre-clip. alpha `0x5bd550`: cdecl `(dest, cel,
dx, dy, w, h, sx, sy, ckey, gdi_ctx)`, blend mode = `*(int*)ECX` (the `0x5bd680` thiscall `this` forwarded
through — best-effort, verify the modes are the valid 0/1/2 per quirk #44 against a capture).

**OPEN RE threads (don't block):** none new for v2. The PAUSED movement-system arc (carried, resumes after
v2): the errands/house ROOM-render path (`plans/controllable-arche-faithful.md` Phase 2a; scene ids
house=1023/errands=1025) → the freeroam hand-off; the A/B portrait FACING (dynamic, lands with the animated
cast); the errands questline `0x4dc510`; the later `+0x200=1` transfer.

## Where we were — ckpt 124

**The dialogue PORTRAITS are UN-MVP'd — the bust RESOLVES per speaker (USER-requested side-fix). New
`src/portrait.{c,h}` + the embedded face table; 982 host pass (+4), 0 fail, 6 skip; one commit `ce1af81`.
Montage on the feed.**

**The fix.** The portrait was hardcoded to `g_ar_sprite_slots[663]` — a WRONG character (face-table head
100000104, face 0xc), so EVERY speaker showed the same bust.  `src/portrait.c` ports the `0x49d6e0`
face-table lookup: `portrait_resolve(head_state, face_id, variant)` scans the embedded `DAT_006b6568` for
the `(head, face)` record → the portrait pool-slot (the `g_ar_sprite_slots`/`DAT_008a760c` index; -1 = no
record = retail's no-portrait path).  `cutscene.c` maps speaker → head-state + resolves the slot per line;
`dialogue_box` carries `portrait_slot` (reset -1 by `dialogue_arm`); `main.c game_render_dialogue` blits
the resolved slot (with the existing cross-fade).

**The data (HARNESS-GROUND-TRUTHED, `runs/portrait-gt`).** Two unknowns resolved by a Frida field-spec on
`0x49d6e0` (`tools/flow/portrait_face_fields.json`, driven by the control-path nav):
- the speaker→head-state map (the face-table key dword[0]): Arche (code 0xc35a) = 100000101, Father
  (0xc3dc) = 100000211, Mother (0xc440) = 100000212 — constant per character.
- the VARIANT (the ALIGNMENT fix, commit `1a527cb`): `bVar4`/var-C is FALSE (`in_ECX+0x2f0`==0 every line);
  the A (`+0x8`) vs B (`+0xa`) choice is the speaker's body-facing (`0x49d6e0:143`, body+0x2c==3 → A).  It
  is DYNAMIC AND the variants are DIFFERENT busts/SIZES (Father A=676 160x176 vs B=683 176x144) — using one
  column (B) rendered Father L1 as the squished 176x144 (the misaligned overlay the USER flagged).  RE'd
  the per-line variant for ALL 18 lines by harness-reading the RESOLVED `+0x84` off the beat-runner thunk
  `0x439680` (NO lag, no loss at the room transition — `runs/portrait-gt` cap4): arrival A,B,B,B,B,B,B,B,A,B
  / house A,A,A,A,A,A,B,A.  Baked into `cutscene_line.pvar`; `arm_current_line` resolves with it.
The face table is embedded by `tools/extract/portrait_face_table.py` → `src/portrait_face_data.{c,h}` (147
records, the `world_tables_data` precedent).

**VERIFIED.** Father/Arche/Mother busts each render the correct per-line bust+pose; Father L1 = slot 676
(160x176) aligned to the box, matching retail f2980 (port|retail montages on the feed).  **USER-CONFIRMED
the portraits look correct.**  Retires `dialogue-portrait-per-speaker`; `dialogue-portrait-facing` reduced
to "`pvar` is captured data, not yet derived from a live cast facing".  OPEN (deferred, USER): a
frame-by-frame TRACE-STUDIO pass to verify the exact expression/pose per line — the ad-hoc comparison
frames were not sim-tick aligned (the portrait cross-fade `0x49c910` blends two busts, so a mid-fade frame
differs); the studio's sim-tick axis is the right tool.

**NEXT MOVE (UNCHANGED from ckpt 123 — the planned arc).** Render the house/errands ROOMS
(`plans/controllable-arche-faithful.md` Phase 2a): room-key → `GW_ROOM_SCENE` → `load_town_scene` (scene
ids confirmed: house `0x334c8`=1023, errands `0x334dc`=1025) → room-keyed reload on the cutscene transition
→ per-room camera → the `map_decode` interior-tileset coverage (the risk) → then the freeroam hand-off.
The portrait un-MVP was a USER-requested side-fix off the ckpt-123 chain landing.

**Module layout (this ckpt):** `src/portrait.{c,h}` (the resolver), `src/portrait_face_data.{c,h}` (the
embedded face table, generated), `src/dialogue.{c,h}` (+`portrait_slot`), `src/cutscene.c` (speaker→head +
per-line resolve), `src/main.c` (renders the resolved slot).  Tools: `tools/extract/portrait_face_table.py`
(generator), `tools/flow/portrait_face_fields.json` (the capture spec).  Tests: `test_portrait.c` (4).
Artifacts (gitignored): `runs/portrait-gt/` (cap/cap2/cap3 + verify/*.png).

**OPEN RE threads (don't block):** the A/B portrait FACING (dynamic; lands with the animated cast); carried
from ckpt 123 — the errands questline `0x4dc510` flag machinery, the later `+0x200=1` transfer, the
room-render path size.

## Where we were — ckpt 123

**The town-intro CUTSCENE now CHAINS arrival→house — the room-key swap is PORTED + BEHAVIORALLY VERIFIED.
`src/cutscene.{c,h}` grew from a single-script driver to a multi-ROOM sequencer; 978 host pass (+4), 0 fail,
6 skip; one commit `daa1f65` direct to master. Montage on the feed.**

**The chip.** `cutscene.c` walks a ROOMS list modeling the room-key swap (`0x401d40` stages the next key to
map+0x900/4/8; `0x402030` commits it to `room_state+0x4024`; `0x4d7d80` re-dispatches on the new key — the
case `return 2` path). The harness-verified chain (quirk #103): arrival `0x334be` (10 lines) → house
`0x334c8` (8 lines) → ENDS at the errands boundary `0x334dc` (= the freeroam control hand-off point). On a
room's last line it COMMITs the next room key (`room_idx++`) + arms its line 0; past the last room it
completes. New API: a `cutscene_room` struct, `cutscene_town_house`/`_chain`, `cutscene_room_key`;
`cutscene_arm` now takes a room chain (the tests + `main.c` updated).

**The house script (RE'd from `0x4d7d80` case 0x334c8, decompile lines 1029-1218).** 8 dialogue lines
(`0x49d6e0` calls), text VAs 0x86d390..0x86d1dc, all unvoiced (voice 0). Speakers are actor ids resolved to
dramatist names — 0x5f5e165=Arche, 0x5f5e1d3=Father, 0x5f5e1d4=Mother — CONFIRMED against the arrival's
known speakers (the arrival uses the same `0x556eb0(id)` actors). Order: Arche/Arche/Mother/Father/Father/
Father → [emote beat `0x401e60`, skipped — `cutscene-beat-runner`] → Arche/Mother.

**VERIFIED (behavioral, `runs/cutscene-verify`).** A seed-pinned replay (the ckpt-121 nav-zspam extended
with Z presses out to flip 13000) drove the LIVE chain through all 18 lines and logged "town-intro cutscene
chain COMPLETE @hold=11365 (reached errands boundary 0x334dc → control hand-off)". Captured house frames
(f8400 "Arche: Hee, there's even an item shop"; f9900 "Arche's Father: Mm-hmm. I'm hoping I can make a
living as a shopkeeper") render correct text + name. KNOWN (tagged debts, NOT bugs): the portrait stays the
Father bust (`dialogue-portrait-per-speaker`); the backdrop is still the town scene — NEW
`PORT-DEBT(cutscene-room-render)`.

**Ledger correction.** `0x439690`/`0x49d6e0` were `FUN_` form in the ckpt-121 cutscene.c (over-claiming
"ported"); the sequencer REDUCES a slice of the beat-runner + captures the line-setup ARGS — it doesn't
port either function → converted to bare VAs (touched 209→207, tested 204→202; the correct count).

**NEXT MOVE — the errands room = the freeroam (a SCOPE DECISION pending with the USER).** Two findings
reshape the plan's "errands + short opening dialogue → stop sequencer, freeroam":
1. The errands room `0x334dc` is handled by the SEPARATE dispatcher `0x4dc510` (21 KB) — a flag-gated
   QUESTLINE (gates on `scene[1]==0xd2`; its own dialogue API `0x4a5ee0`; storage-room/Sana sub-scenes),
   NOT a linear cutscene. The errands room IS gameplay/freeroam (quirk #103 finding #4).
2. NEITHER the house nor errands ROOM is RENDERED. The committed room key `room_state+0x4024` drives an
   unported map-load path (the `0x585ae0`/`0x586010` family, 14 `+0x4024` consumers). So the house lines
   currently play over the TOWN-ARRIVAL backdrop.
The freeroam hand-off (stop the sequencer + run `character_step` on live `axis_held`, the `+0x200==0`
char-AI path — the mover is DONE bit-exact, live input DONE) is only NON-FAKE once the errands room
RENDERS — else Arche walks over the town backdrop, recreating the ckpt-120 "wrong scene" the USER removed.
**Recommendation: the ROOM-RENDER path is the faithful foundation** (port the room-key→map-load/decode so
house+errands backdrops render → fixes the house-dialogue backdrop AND unblocks a real freeroam); the
freeroam hand-off + DROP-`+0x200=1` then follow. Asking the USER to confirm this order vs alternatives.
Plan: `plans/controllable-arche-faithful.md` Phase 2.

**Module layout (this ckpt):** `src/cutscene.{c,h}` (multi-room sequencer + arrival/house line tables +
the room chain), `src/main.c` (arms `cutscene_town_chain`, the completion log = "reached errands boundary"),
`tests/test_cutscene.c` (9 tests: tables, chain, room transition, room_key, full-chain completion).
Throwaway verify artifacts (gitignored): `runs/cutscene-verify/` (`nav-zspam-ext.jsonl`, `house/*.png`).

**OPEN RE threads (don't block):** (a) the errands questline `0x4dc510` flag machinery (needs harness
verification, not static reading — the `scene[1]==0xd2` gate vs the staged `(0x334dc,1,1)`); (b) the LATER
`+0x200=1` transfer (post-errands → Sana, a separate capture); (c) the room-render path SIZE (a reuse of
the town `map_decode` with the room's map data, or deeply entangled — investigate when the render chip
starts). Carried: butterfly chip-1 drift visual-verify; LIVE-input windowed visual-verify (title menu
arrows + Z). Debt: + `cutscene-room-render`, retired the SCRIPT-chain part of `cutscene-scene-chain`;
carried char-control-trigger / cutscene-party-chars / dialogue-portrait-per-speaker / keybind-config /
cutscene-beat-runner / char-run-trigger / char-walk-tuning / char-collision-mover / char-input-autorepeat /
char-jump-fall-grav-source / held-axis-array-b / effect-color-variant.

## Where we were — ckpt 122

**The control-transfer PATH is HARNESS-VERIFIED — the ckpt-121 "DO FIRST" step is done; the
static-vs-live conflict is RESOLVED and the porting model CORRECTED. Pure RE/harness, NO port code;
974 host pass (unchanged). quirk #103; artifacts `runs/control-path-gt/` (montage on the feed).**
Seed-pinned `--lockstep --no-turbo` retail drove the PROVEN ckpt-112 nav (Z-spam from `game_enter`,
incl the two `id=3` LEFT presses @830/865 that pass the new-game submenu — the uniform-Z nav stalled
without them) extended to flip 8392, under a per-Flip field spec reading off
`room_state = *(*(0x8a9b50)+0x2784)`.

**The four findings (full writeup: quirk #103):**
1. **3-ROOM CHAIN, confirmed.** The committed room key `room_state+0x4024` (staged by `FUN_00401d40`,
   committed by `FUN_00402030`) swaps **`0x334be` arrival (flip 1430/tick 1) → `0x334c8` house (flip
   3661/tick 1103) → `0x334dc` errands (flip 4270/tick 1406)**. The stager fired exactly twice
   (`staged_key 0x334c8`@3659, `0x334dc`@4268). The static-RE room sequence is REAL.
2. **LIGHT room-key swap, NOT a full reload.** ONE `game_enter` (~1429); `room_state`, leader `+0x200c`
   (`0xd1dcc58`), entity (`slot+0x9f4`, code `0xc35a`), flag table `+0x40ac`, `+0x158a4` all hold
   CONSTANT across both swaps. ckpt-112's "no reload / entities persist" right; "same scene" wrong.
3. **CONTROL = `entity+0x200 == 0` (char-AI), NOT `+0x200=1`** — the ckpt-114 polarity open RESOLVED.
   A held-axis walk in the errands room drove Arche's leader-chain body bit-exact: held-RIGHT (flip
   4400-4900) `wx 19200→73800` facing 1, held-LEFT (5000-5500) →`14640` facing 3, the exact ported walk
   accel/cap — all while `+0x200`==0 and `+0x158a4` (input-mgr) non-null. Matches `0x46cd70`'s dispatch
   (`+0x200==0` → char AI `0x478ba0` reads the held axis). The `0x41e070`/`0x4c6830` `+0x200=1` setters
   are a LATER/different control point (party / end-of-day `4d7d80` sites B/C), NOT the first freeroam.
4. **The errands room `0x334dc` IS the freeroam.** Frames show a multi-floor HOUSE INTERIOR with the
   "PLAYER" marker over Arche + the full HUD (portrait/"Arche Lv1"/HP/item bar) — = ckpt-112's
   "PLAYER!@4500", correctly located in the errands room. USER-confirmed: "a house with mom and dad and
   you run some errands and there's short dialogue at the start." Z-spam STALLS at errands (it's
   gameplay, not dialogue) — the stall IS the control boundary.

**NEXT MOVE — PORT the verified chain (the active arc):** extend `src/cutscene.{c,h}` to chain the 3 room
scripts (arrival ✓ → house `0x334c8` 8 lines, text VAs 0x86d390..0x86d1dc → errands `0x334dc` + its short
opening dialogue) via the LIGHT room-key swap (`0x401d40` stage → `0x402030` commit, no full reload —
model it as a scene/script change within the live town); at the errands room STOP the sequencer and run
`character_step` on live `g_game_drive.input.axis_held` (the `+0x200==0` char-AI path) + the
`input_live.c` keyboard producer — the faithful replacement for the ckpt-120 `CHAR_CONTROL_ARM_FRAMES`
MVP arm. DROP the `+0x200=1` model for the first freeroam. Plan corrected:
`plans/controllable-arche-faithful.md` "Phase 2 VERIFIED".

**Field-spec offsets (off `room_state = *(*(0x8a9b50)+0x2784)`; the durable read for any control-path
capture):** `+0x4024/4028/402c` committed room-key triple; `+0x40ac` story-flag table (`{key,val,?}`×
count@`+0x600`, scanned by `FUN_0041e2f0` for `0x5f76805`); `+0x200c` leader slot → `slot+0x9f4` entity
/ `+0x9f0` handle / `+0x9c4` active; entity `+0x1d4` code (`0xc35a`), `+0x200` control (0=char-AI), `+0x158a4`
input-mgr, `+0x40` body (`+4` wx/`+8` wy/`+0x2c` facing); global mode `*(*(0x8a9b50))+0x1030`=2.
Spec file: `tools/flow/control_handoff_fields.json`.

**OPEN RE threads (don't block the port):** (a) the LATER `+0x200=1` transfer (after the errands
complete → town flag-0xd2 → Sana scene) — needs a walk-to-trigger / interact nav, a separate capture;
(b) whether the char-AI is actively SUPPRESSED during the arrival/house cutscenes or merely un-fed
(held-input not tested there) — refines whether the port's "switch to freeroam" is a gate or just
"stop scripting her". Carried: butterfly chip-1 drift visual-verify; LIVE-input windowed visual-verify
(title menu arrows + Z) + the dialogue advance.

## Where we were — ckpt 121

**UN-MVP'd the movement: 3 of 4 steps landed (the MVP wire REMOVED; FAITHFUL live keyboard input + the
town-arrival DIALOGUE ADVANCE ported + verified). USER COMMITTED to the FULL FAITHFUL control-transfer
chain. 974 host pass (+11 over ckpt 120), 0 fail, 6 skip; 4 commits direct to master.**  The USER's
directive: un-MVP the controllable Arche (the ckpt-120 live-wire put control in the wrong scene via a
measured trigger) — remove it, then build the faithful base (live input + the real dialogue-driven hand-off).

**Step 1 — REMOVED the MVP wire** (`src/main.c`; commit `42e0fc1`).  Deleted the
`g_arche`/`CHAR_CONTROL_ARM_FRAMES` scaffold (the measured-frame controllable-Arche in the settled-town
cutaway).  The mover (`src/character.{c,h}`, bit-exact) is UNTOUCHED — its faithful caller returns with step 4.

**Step 2 — FAITHFUL LIVE INPUT** (`src/input_live.{c,h}` NEW + `tests/test_input_live.c`; the port of the
per-frame producer `FUN_0046a880`; commit `61a6aaa`).  Each frame: rebuild `input_mgr.axis_held[0..6]` from
the DIK keyboard snapshot (clear-then-set = the `0x46a880:1497-1538` fill + the `0x56a220` release flush) +
post a ring event on each key press/release EDGE (the `0x46a880:1380-1496` push, the SAME ring
`input_trace_replay` injects).  Default keybinds (PORT-DEBT(keybind-config)): arrows → axis[0..3]+ring
1/3/2/4, C → axis[4]+ring 7 (jump), X → axis[5], Z → ring 0x24 (advance).  `main.c`: a single
`feed_input(m, now)` drives the active scene's input — REPLAY wins (`--input-trace`/`--held-trace`, the
deterministic parity path), else the LIVE producer (gated on `g_app_active_flag` = focus); mutually
exclusive so live keys never perturb a capture.  `live_keyboard_snapshot` = `GetAsyncKeyState` (DIK→VK).
6 host tests; boot smoke clean.  **USER VISUAL-VERIFY PENDING** (windowed: title menu navigable with arrows + Z).

**Step 3 — the DIALOGUE ADVANCE** (`src/cutscene.{c,h}` NEW + `tests/test_cutscene.c`; `dialogue.{c,h}`
+`dialogue_awaiting_advance`; commit `8d4c096`).  The port was stuck on dialogue LINE 1.  Now
`src/cutscene.c` sequences the real town-gate family conversation — a 10-line script table reduced from
`0x4d7d80` case 0x334be (lines 33-292) + the beat-runner `0x439690` — Z-advancing through all 10 lines.
Each line = (speaker dramatist NAME VA, line TEXT VA, face, voice), strings read from the user's exe by VA
at runtime (names = dramatist rows 0/4/5: Arche 0x6b6eb0 / Father 0x6b6f80 / Mother 0x6b6fb4; text
0x86d58c..0x86d3d4).  `cutscene_step` advances on Z (ring 0x24) ONLY when the line is fully typed
(`dialogue_awaiting_advance` = `0x439690:1004`); Z while typing is ignored (faithful — no skip).  Armed at
the SAME measured trigger as the old line-1 arm (line 1 stays bit-exact).  REPLAY-VERIFIED end-to-end
(`runs/cutscene-verify`: game_enter@1116 → cutscene_arm@hold 1283 → f2600 "Father: Ahh…" → f3150 "Arche:
Yay" → f3700 "Mother: We haven…"; montage on the feed).  5 host tests.  KNOWN: the portrait stays the
Father bust (PORT-DEBT(dialogue-portrait-per-speaker)).  Retires `dialogue-line-table`.

**Step 4 — the FULL FAITHFUL CONTROL TRANSFER (USER-committed; the active arc, NOT started).**  The RE
(two general-purpose subagent maps of `0x4d7d80`/`0x401d40`/`0x41e070`):
- the hand-off is NOT a few lines past the arrival.  `FUN_00401d40(scene_id,…)` stages a ROOM TRANSITION
  (writes the next room-lookup key into the map object `+0x900`→committed `+0x4024`; the engine reloads).
  Narrative spine: arrival `0x334be` flag-0 (10 lines) → load room **0x334c8** (house interior; 8 lines,
  text 0x86d390..0x86d1dc) → load room **0x334dc** (morning errands, the SEPARATE dispatcher
  `FUN_004dc510`; advances story-flag `0x5f76805` 0→0xd2) → back to **town 0x334be flag 0xd2** → the
  Sana-walk-home scene (lines 295-481) → **the control transfer**.
- the transfer (`4d7d80.c:449-463`, the inlined `0x41e070`/`0x4c6830` idiom): `FUN_00413b20(handle)` →
  `FUN_004c63a0(actor,1)` (release from the cutscene band) → `FUN_004cc250` guard → **`*(entity+0x200)=1`**
  (the master player-controlled flag; `+0x158a4=0` clears the AI script) → `FUN_0041e180(1)` (clear the cmd
  ring) → `FUN_0041e280()` (re-bind DirectInput to the entity) → `FUN_0041dc90()` (recompute the party band).
  Returns NOT 2 → no room reload; control stays in the town.  Two later sites (B 719-733 flag 0x140, C
  882-896 resume==3) are end-of-day / post-school — NOT the first.
- **CONFLICT to resolve FIRST (don't guess — harness):** ckpt-112 observed retail reaching control via
  Z-spam with ONE game_enter and NO map reload.  The static read implies room transitions.  Either those
  keys map to the SAME town map (camera-only), or the live path is shorter.  **DO FIRST:** a Frida
  field-spec on the scene-controller room key (`*(0x8a9b50+0x1038)[0]` / map `+0x4024`) + Arche's `+0x200`
  + flag `0x5f76805`, Z-spam retail from game_enter to the hand-off → the ACTUAL live sequence.  THEN port
  the (possibly-reduced) chain + the room-transition system if needed + the transfer + wire `character_step`
  at the real `+0x200=1` transition.

**NEXT MOVE:** the harness verification above (Phase 2 of `plans/controllable-arche-faithful.md`, rewritten
this ckpt).  The control MECHANISM is small + clear; reaching retail's exact LOCATION is the arc.  The
mover is DONE; the render is the static cutscene-cast slot (Arche slides, no walk-cycle —
PORT-DEBT(cutscene-party-chars), the animation system is a later Phase 3, out of the un-MVP scope).

**Module layout (this ckpt):** `src/input_live.{c,h}` (live producer), `src/cutscene.{c,h}` (the
town-arrival sequencer), `src/main.c` (`feed_input` + cutscene wiring; MVP wire removed), `src/dialogue.{c,h}`
(+`dialogue_awaiting_advance`).  Tests: `test_input_live.c` (6), `test_cutscene.c` (5).  Throwaway verify
artifacts (gitignored): `runs/cutscene-verify/`.

**OPEN (USER):** (a) windowed visual-verify of LIVE input (title menu arrows + Z) + the dialogue advance;
(b) butterfly chip-1 drift visual-verify still pending.  Debt: cutscene-beat-runner / cutscene-scene-chain /
dialogue-portrait-per-speaker / keybind-config (new); char-control-trigger / cutscene-party-chars /
char-run-trigger / char-walk-tuning / char-collision-mover / char-input-autorepeat /
char-jump-fall-grav-source / held-axis-array-b / effect-color-variant (carried).

## Where we were — ckpt 120

**PHASE-4 chip 3c — the LIVE WIRE: Arche is CONTROLLABLE ON SCREEN. `character_step` gets its FIRST
live caller; held-axis input drives Arche walking in the settled town. 963 host pass (unchanged —
`src/main.c` wiring only). USER VISUAL-VERIFY PENDING (montages pushed to the feed).**  The movement-
system MILESTONE: the chip-3a/b walk physics (host-validated bit-exact across walk/run/jump/windup) now
drive Arche's rendered sprite live.  Module: `src/main.c` (the include + the `g_arche`/`g_arche_slot`/
`g_arche_armed` globals + `CHAR_CONTROL_ARM_FRAMES` + the `enter_game` slot-find + the
`game_actor_update` step+mirror).  Capture artifacts (gitignored): `runs/livewire/` (`walk.jsonl`/
`walk2.jsonl` traces, `arche_walk_scene.png`/`arche_walk2_montage.png`).

**The wire (mirrors the butterfly pattern exactly).**  Arche is the cutscene-cast EFFECT actor (code
`0xc35a`, found by scanning `g_effects` after `actor_spawn_cutscene_cast`; slot 18, body bank 0x8b,
world 41600/45600, facing 1, idle clip).  At a MEASURED control-transfer frame (`CHAR_CONTROL_ARM_FRAMES`
=200 flips post-game_enter; PORT-DEBT(char-control-trigger)) `game_actor_update` (already sim-tick-gated)
`character_init`s `g_arche` from her settled render pos, then each sim-tick runs `character_step(&g_arche,
m->axis_held, m->axis_held[4], /*run=*/0)` (`m`=`g_game_drive.input`; `axis_held[0..3]` align with
`CHAR_AXIS_*`, `[4]`=jump C) and mirrors `world_x/world_y/facing` into her render-state — exactly how
`butterfly_step`'s output mirrors into the EFFECT actors.

**Input path (the verification is the REPLAY capture, the established model).**  The port has NO live
in-game keyboard producer (WM_KEYDOWN is a no-op), so input comes via the `held_trace`/`input_trace`
REPLAY — the same path walk/jump/dash were all validated on.  `held_trace_replay` fills `axis_held[0..3]`
(walk); jump (`[4]`) needs a held_trace extension (deferred — the walk is the milestone).

**VERIFIED (the capture, `runs/livewire`).**  Drove the port to the settled town (nav
`runs/trace-studio/intro-1/edit.trace.port.jsonl`; game_enter@1116; the scripted camera pans LEFT from
cur_x 128000 to **12800**, settling ~flip 2097 with Arche at ~45% across frame).  Held-LEFT-then-RIGHT
(`walk2.jsonl`, frames 2110-2390, the clean window post-pan / pre-dialogue@2398): Arche walks left to
Barnard (world ~27600) then back right, smooth accel/decel, NO position glitch.  Montages on the feed.

**KNOWN DEFERRED (render polish, NOT the mover — the mover is bit-exact).**
- She SLIDES on the IDLE clip (no walk-cycle animation) and stays RIGHT-FACING when walking left: the
  render's `facing==3` mirror selects `frame_base + flip_table[bank]` (a pre-mirrored FRAME, not a blit
  flip), and bank 0x8b has no mirror-frame registered (she spawned facing-right).  No position glitch
  (the off_x reflection is benign).  Both = PORT-DEBT(cutscene-party-chars) (the multi-part party-band
  render `0x4997b0` + her directional/walk frames).  The `facing` DATA is written correctly, so when the
  animated render lands it mirrors with no further wiring.
- `run`=0 (no live double-tap source under held-axis replay → PORT-DEBT(char-run-trigger)).

**NEXT — the USER has SET ASIDE this MVP (ckpt-120 directive 2026-06-11): go FAITHFUL next session.**
The MVP wire proved the seams (the actor/input/render path works end-to-end) but is a throwaway scaffold
— measured trigger + replay-only input + static render.  The USER's call: *don't build the animation
system on the MVP (annoying to un-MVP); go straight to the actual scene where Arche is genuinely
controllable, plus faithful LIVE input so movement is meaningfully testable in the port.*  Full plan:
**`docs/plans/controllable-arche-faithful.md`**.  Three phases (the mover itself is DONE — bit-exact):
1. **Faithful LIVE input** (the USER's emphasis) — port the held-axis producer `0x46a880` + the ring
   producer so REAL keyboard fills `input_mgr.axis_held`/the ring (the port has no live keyboard — WM_KEYDOWN
   is a no-op), alongside the deterministic replay (which stays the parity path).  Most self-contained.
2. **The REAL control hand-off** — dialogue chip 4 (`0x4d7d80`→`0x439690` beat-runner + the ~15-line
   script) → the `entity+0x200=1` transfer (`0x41e070`/`0x4c6830`) → `character_step` wired at the REAL
   transition, REPLACING the `CHAR_CONTROL_ARM_FRAMES` MVP scaffold + the dash trigger `0x479e70`.
3. **The animation system** — on the faithful party-band render `0x4997b0` (Arche's directional/walk-cycle
   frames + the facing mirror), NOT the MVP static slot.  Retires cutscene-party-chars.

**OPEN (USER):** the ckpt-120 MVP commit stays in history as the seam-proving prototype — REVERTABLE for a
clean slate if the USER wants (Phase 2 supersedes it regardless; ask before reverting).  Butterfly chip-1
drift visual-verify still pending.  Debt: PORT-DEBT(char-control-trigger / char-run-trigger /
char-jump-fall-grav-source / char-walk-tuning / char-collision-mover / char-input-autorepeat /
cutscene-party-chars), PORT-DEBT(held-axis-array-b), PORT-DEBT(effect-color-variant).

## Where we were — ckpt 119

**PHASE-4 chip 3b — the jump WINDUP is PORTED + BIT-EXACT (the launch-anticipation delay between the
jump trigger and the impulse). 963 host pass (+1).**  The jump execute enters the airborne state
IMMEDIATELY but the body stays STATIONARY for exactly **4 sim-ticks** (a visible launch crouch, ~8
flips) before the impulse fires.  RE'd from the `0x442a70:834-841` case-3 sub-state-0 branch and
ground-truthed bit-exact off the EXISTING `capjump-ring2` capture (no fresh capture needed).  Modules:
`src/character.{c,h}` (+ `jump_sub`/`jump_ctr` + the windup branch), `tests/test_character.c`
(+`character_jump_windup`; the arc/held-rise tests got the windup prefix), `tests/test_main.c`.
Writeup: **engine-quirk #102** (the windup bullet).

**The windup LAW (`0x442a70` case 3 sub-state 0, the decompile-decisive branch):**
- the execute (`cmd[2]==7` → `0x426f50(body,3)`) sets `body+0x38`=3 (main), `+0x3a`=0 (sub), `+0x3c`=0
  (counter) — a 3-write setter, so the windup count is independent of prior state.
- case 3 sub 0: `counter = counter + 1; if (4 < counter) { vvel := in_ECX[0x5667]; sub := 1; counter := 0 }`.
  So counter 0→1 on the entry tick, 1→2, 2→3, 3→4 (all stationary, `vvel`=0), then 4→5 (`4<5`) → impulse.
  **4 stationary windup ticks, launch on the 5th.**

**The ground truth (the `bstate` field already had it).**  `capjump-ring2`'s `bstate` reads `body+0x38`
as u32 = main | sub<<16.  Decoding the first jump: flips **4602-4609 = (main 3, sub 0, vvel 0)** = the 4
windup ticks (8 flips = 4 sim-ticks, body updates every 2 flips), flip **4610 = (main 3, sub 1, vvel
−76000)** = the impulse.  The earlier `jump_arc.py` keyed on `vvel!=0`, so the windup was invisible to
the arc extraction — but it was always in the capture.

**The port (`character.c`).**  Added `jump_sub`/`jump_ctr` (mirror `body+0x3a`/`+0x3c`) + the windup
branch (`CHAR_JUMP_WINDUP_THRESH`=4): the jump rising edge enters airborne sub-0 with a fresh counter;
the windup block runs THIS tick (the entry tick is windup tick 1); on the tick the counter exceeds 4
the impulse fires (+ one fall-grav step → −76000) and sub advances to 1.  The real sub-states 1/2/3
(transient/rise/fall anim bookkeeping) collapse to the port's existing vvel-sign branch; the main-state-4
landing recovery is subsumed by the flat ground clamp.  `test_character_jump_windup` asserts the entry
tick (airborne, sub 0, ctr 1, vvel 0, stationary), the 3 more windup ticks (ctr 2,3,4), and the launch
tick (sub 1, ctr 0, vvel −76000, wy 51200) bit-exact vs the capture.

**NEXT (chip 3c — the milestone):**
1. **The LIVE wire** — the chip-4 freeroam hand-off (dialogue chip 4 → the `entity+0x200` control
   transfer) gives `character_step` its first live caller in `game_actor_update` → Arche walks/jumps/
   dashes on screen, the chip-2 collision mover/probes get a live grounded actor → USER visual-verify
   (the milestone).  The live wire also retires PORT-DEBT(char-run-trigger) (real `0x479e70` ring access)
   + char-walk-tuning (read `in_ECX[…]` off the live entity) + char-collision-mover (the real terrain
   surface + the held-jump ceiling).

**OPEN (USER):** butterfly chip-1 drift visual-verify still pending (trace-studio `intro-1` ~1580-1670).
Debt: PORT-DEBT(char-run-trigger / char-jump-fall-grav-source / char-walk-tuning / char-collision-mover
/ char-input-autorepeat), PORT-DEBT(held-axis-array-b), PORT-DEBT(effect-color-variant).

## Where we were — ckpt 118

**PHASE-4 chip 3b — Arche's DASH (run) is PORTED + FIELD-EXACT, validated BIT-EXACT against a fresh
ring-double-tap capture. 962 host pass (+1).**  The run is a small, fully-understood delta on the walk
(same `body+0x28` accumulator, same `0x445db0` clamp-ramp) — RE'd from the `0x442a70` case-0x75 run
branch + the live const band (ckpt 117), then validated tick-for-tick vs retail's captured per-tick body.
Modules: `src/character.{c,h}` (the `run` arg + the run accel/cap branch), `tests/test_character.c`
(+`character_run_ramp`), `tests/test_main.c`; throwaway capture artifacts `runs/runjump-gt/{capdash2,
dash-ring2.jsonl,dash-held.jsonl,dashframes.txt}` + `tools/flow/dash_fields.json`.  Writeup:
**engine-quirk #102** (the run-physics bullet); debt **PORT-DEBT(char-run-trigger)**.

**The chip-3b RUN BLOCKER is CLEARED (the ring double-tap fires).**  `0x479e70` matches two direction
ring events (id 2=LEFT / 4=RIGHT) with `flag==1` in the window `*(*0x8a6e80+0xf8)`, marking found events
**by slot index** (`0x479960`'s `local_100` scratch), NOT by timestamp — so injecting `ids:[4,4]` (two
id-4 events → ring slots 63/62, same ts) is a VALID double-tap.  Held RIGHT (the `--held-trace` axis)
sustains it (`local_608[0]==6` self-sustain while held).  The detection doesn't consume the events
(`param_9=0`), and events linger a few flips (32 ms ≪ window) → timing is forgiving.

**Capture method (the flaky→clean lesson).**  `capdash` (2-VA field spec + the held-leaf hook) STALLED —
`sim_tick=0` the whole run, the leader chain null, freeroam never reached (the heavy hooking tripped the
lockstep stall-breaker / dialogue desync).  `capdash2` (a **lean 1-VA** `dash_fields.json` + the **PROVEN
`runs/freeroam-walk` nav** `trace-nav.jsonl` + the id-4 double-taps) reached freeroam clean and the dash
fired: cmd0 **2 (walk)→6 (run)**, `hvel` ramped to **48000**, `wx` to **+480/tick**.  LESSON: fewer hooks
+ a known-good nav when a capture must drive deep through dialogue under lockstep.

**The run LAW (`0x442a70` case 0x75, the cmd[0]=5/6 branch — bit-exact target = retail's captured bytes):**
- **RUN cap `in_ECX[0x5664]` = 48000** → dwx cap **±480** (exactly 2× the walk's ±240).
- **TWO-PHASE accel:** `442a70:998` picks `in_ECX[0x565d]`=**3200** only while `hvel < param_3` where
  param_3 is still the WALK cap 24000 (line 950, before it's reassigned to 48000 at `:1001`); at/above
  24000 the accel falls to the default walk accel `in_ECX[0x565c]`=**1600**.  Captured per-tick (RIGHT
  held from rest): `1600,3200` (walk, cmd0=2) → double-tap latches cmd0=6 → `6400,9600,12800,16000,
  19200,22400,25600` (+3200) → `27200,28800,…,46400,48000` (+1600 to the cap).
- **Brake = the WALK brake −800** (`local_20`=`in_ECX[0x565e]`, unchanged in the run branch).  Releasing
  the dash while still holding the dir decays 48000→24000 at −800/tick (the `0x445db0` over-cap path,
  `+local_18`), then walks at 24000.  Same accumulator + same ramp as the walk → only cap+accel differ.

**The port (`character_step(c, axis_held, jump_held, run)`).**  Added the `run` arg = the RESOLVED
cmd[0]==5/6 (the AI's `0x479e70` double-tap detection is INPUT-layer, deferred to the live wire →
PORT-DEBT(char-run-trigger); mirrors how `jump_held` is the resolved button).  The accelerate branch:
`cap = run ? 48000 : 24000`, `accel = (run && |vel| < 24000) ? 3200 : 1600`, with the over-cap brake when
`|vel| > cap`.  `test_character_run_ramp` asserts the captured `(hvel, worldX)` bytes tick-for-tick + the
over-cap decay.  The existing 6 character tests got the 4th arg (`run=0`) — all still pass.

**NEXT (chip 3b/3c, in order):**
1. **RE + port the jump WINDUP** (the ~7-flip execute→launch delay = case-3 sub-state-0 counter>4,
   `0x442a70:835-841`) — a launch lag invisible to the arc but visible live (PORT-DEBT(char-jump-variable-height)).
2. **The LIVE wire** — the chip-4 freeroam hand-off (dialogue chip 4 → the `entity+0x200` control transfer)
   gives `character_step` its first live caller in `game_actor_update` → Arche walks/jumps/dashes on screen,
   the chip-2 collision mover/probes get a live grounded actor → USER visual-verify (the milestone).  The
   live wire also retires PORT-DEBT(char-run-trigger) (real `0x479e70` ring access) + char-walk-tuning
   (read `in_ECX[…]` off the live entity) + char-collision-mover.

**OPEN (USER):** butterfly chip-1 drift visual-verify still pending (trace-studio `intro-1` ~1580-1670).
Debt: PORT-DEBT(char-run-trigger / char-jump-variable-height / char-jump-fall-grav-source / char-walk-tuning
/ char-collision-mover / char-input-autorepeat), PORT-DEBT(held-axis-array-b), PORT-DEBT(effect-color-variant).

## Where we were — ckpt 117

**PHASE-4 chip 3b — Arche's JUMP is PORTED + FIELD-EXACT (BOTH the short hop and the variable-height
held rise), and the move-tuning consts are CAPTURED LIVE off her entity (resolving an earlier decompile
mis-read). 961 host pass (+3).**  The vertical airborne integrator now lives in `src/character.{c,h}`
alongside the chip-3a walk; the captured SHORT-HOP arc + the HELD high-jump RISE are reproduced bit-exact.  Modules: `src/character.{c,h}` (+ `world_y`/`vvel`/`airborne`
+ the jump integrator), `tests/test_character.c` (+`character_jump_arc`, `character_jump_edge_and_ground`),
`tests/test_main.c`; throwaway specs `tools/flow/jump_consts_fields.json` + `jump_constband_fields.json`;
docs (`engine-quirks.md` #102 amended, `port-debt.md` rows).  Writeup: **engine-quirk #102** (ckpt 117).

**The jump model (RE'd from `0x442a70` case 3 + the bit-exact arc + the captured consts):**
- `character_step(c, axis_held, jump_held)` runs the vertical integrator EVERY tick alongside the walk.
- **launch** on the jump rising edge (grounded): `vvel = impulse (-80000)`; step `worldY += vvel/100`;
  then ONE fall-grav step (the grav was selected from the pre-impulse `vvel==0` = fall branch) → the
  first sampled vvel is **−76000**, not −80000.
- **rise** (`vvel<0`): `worldY += vvel/100`; `vvel += rise_grav`, VARIABLE-HEIGHT — jump HELD → 2000
  (floaty high jump), RELEASED → 8000 (short hop).
- **fall** (`vvel>=0`): `worldY += vvel/100`; `vvel += 4000` (button-independent).
- **land**: a downward step that would penetrate `ground_y` → clamp `worldY=ground_y`, `vvel=0`,
  grounded (the flat reduction of the vertical collision mover `0x54e5c0`; town street is flat).

**The consts CAPTURED off Arche's entity (`in_ECX[idx]` = entity byte `idx*4`; `runs/runjump-gt/capconsts`
+ `capband`, read live @flip 1500):** impulse `[0x5667]`=−80000, rise grav HELD `[0x5668]`=2000, rise grav
FREE `[0x5669]`=8000, walk cap `[0x565b]`=24000, walk accel `[0x565c]`=1600, run accel `[0x565d]`=3200,
walk brake `[0x565e]`=−800.  **This CONFIRMS the ckpt-115 walk-tuning hypothesis** and corrects an
earlier line-by-line decompile read that mis-mapped `[0x565b]/[0x565e]` to fall-grav/terminal.  The fall
grav (4000) is NOT in the move-tuning band `0x565a..0x566f` → a global/derived gravity (4000 = 8000/2).

**Both jump heights bit-exact (verified):** the SHORT HOP (`capjump-ring2` — the ring execute `cmd[2]=7`
is a ONE-tick event, so `cmd[2]==0` the whole rise → FREE grav 8000) AND the HELD high-jump RISE
(`capheld` — held C via the leaf → `cmd[2]=8` the rise → grav 2000, matches the port's held model for 16
ticks, apex 2.2× higher).  **Two mechanics found in the held capture:** (a) retail's held apex CLAMPS on
a **town CEILING** (~tick 16, wy≈41600; the `wy += vvel/100` relation breaks = a vertical collision clamp
by `0x54e5c0`) — NOT jump physics; the flat-ground port keeps rising (the `char-collision-mover` debt);
(b) **terminal fall velocity = 64000** (the held fall plateaus at 64000 for 8 ticks) — ported as
`CHAR_JUMP_FALL_TERMINAL` (the short hop reaches exactly 64000 at landing, so unaffected).

**METHOD LESSON (durable):** `0x442a70` is a 12 KB shared integrator (walk+run+jump+skills+collision+anim)
whose Ghidra decompile reuses `param_2`/`param_3`/`local_20` across vertical and horizontal terms and has
control-flow reconstruction artifacts.  RE the STRUCTURE from it, but PIN the values + index→constant
provenance with a LIVE const capture — never a line-by-line port.  The bit-exact arc + the captured consts
together are the ground truth (this is how the `[0x565b]` fall-grav-vs-cap ambiguity was resolved).

**NEXT (chip 3b/3c, in order):**
1. **Capture + port the DASH (run)** — inject two direction ring presses within the double-tap window
   then hold (`ids:[4]` ×2 + the held RIGHT axis) → cmd[0]=5/6 → capture the run cap (> the walk's 240;
   run accel `[0x565d]`=3200 already captured) → port.
2. **RE + port the jump WINDUP** (the ~7-flip execute→launch delay = case-3 sub-state-0 counter>4) — a
   few-tick launch lag, invisible to the arc but visible live (PORT-DEBT(char-jump-variable-height)).
3. **The LIVE wire** — the chip-4 freeroam hand-off (dialogue chip 4 → the `entity+0x200` control
   transfer) gives `character_step` its first live caller in `game_actor_update` → Arche walks/jumps on
   screen, the chip-2 collision mover/probes get a live grounded actor → USER visual-verify.

**OPEN (USER):** butterfly chip-1 drift visual-verify still pending (trace-studio `intro-1` ~1580-1670).
Debt: PORT-DEBT(char-run / char-jump-variable-height / char-jump-fall-grav-source / char-walk-tuning /
char-collision-mover / char-input-autorepeat), PORT-DEBT(held-axis-array-b), PORT-DEBT(effect-color-variant).

## Where we were — ckpt 116

**PHASE-4 chip 3b — the run/jump BLOCKER is RESOLVED and Arche's JUMP is captured BIT-EXACT in the
TOWN freeroam. Pure ground-truth (no port code yet). 958 host pass (unchanged).**  The ckpt-115
"dash/jump need a platforming/dungeon scene" hypothesis is **REFUTED** — jump/dash are sourced from
the discrete EVENT RING, and the chip-3b captures only injected the HELD-AXIS, so the ring events
that EXECUTE the jump/dash were never posted. A harness-injection gap, not a scene gate. Full
writeup: **engine-quirk #102** (amended). Artifacts: `runs/runjump-gt/` — `jump-ring.jsonl` (the
nav + jump-as-ring-event trace), `capjump-ring2/` (the capture), `jump_arc.py` + `jump_arc.png`
(extraction + plot, pushed to the feed).

**The RE (decompile-decisive).**  The apply `0x442a70` reads the 8-int command block and executes
the jump on **`cmd[2]==7`** (line 801: → `FUN_00426f50(body,3)` airborne state + the impulse) — it
NEVER consumes **`cmd[2]==8`** (only a generic "any cmd set" check at `:123`). `cmd[2]=8` is the C
button's HELD-array marker (`0x478ba0:483` reads `input-mgr+0x124` → `cmd[2]=(prev==9)+8`) = the
hold-to-rise / variable-height input. `cmd[2]=7` (execute) comes from `0x478ba0:287` matching a
discrete **ring** event: `FUN_00479960(now,0,800,1,7,…)` scans the ring `input-mgr+0xc` (64 ×
`{id@0,ts@4,flag@8}`, the SAME ring `--input-trace` fills for the Z-advance id `0x24`) for `id==7`
within an 800 ms window with `flag==1`. The agent's `injectPress` writes `flag=1`, so injecting
`ids:[7]` matches. **Dash (cmd[0]=5/6) is the same gap:** `FUN_00479e70` matches a direction
DOUBLE-TAP in the ring (id 2=LEFT / 4=RIGHT within `*0x8a6e80+0xf8`), unreachable from held injection.

**Empirically confirmed (`runs/runjump-gt/capjump-ring2`).**  Re-captured the town freeroam with the
nav (Z-spam to the inn control transfer) + `{"frame":N,"ids":[7]}` jump presses at 4600/4660/4720/
4780 — ZERO harness changes (the ring injection already existed). Arche jumps: `vvel 0 → −76000`,
`wy 52000 → 47200 (apex) → 52000`, lands clean. **Two byte-identical jumps** (deterministic).

**The jump arc — the bit-exact port target (per sim-tick, from `jump_arc.py`):**
- impulse **vvel = −80000** (the first wy step is −800; the displayed vvel starts −76000), then
  `wy(t+1) = wy(t) + vvel(t)/100` (verified exactly for all 27 ticks).
- gravity is **ASYMMETRIC**: rise decel **+8000/tick**, fall accel **+4000/tick** (a floaty fall ≈
  Arche's reputation; ~27 ticks airtime, apex `wy=47200` = rise **4800** above the `52000` ground).
- ground contact (`wy ≥ 52000` while `vvel > 0`) clamps `wy=52000`, zeroes vvel.
- the apex/fall branch is the body+0x38==3 airborne sub-FSM (`0x442a70:832-877`, the `-20000` vvel
  threshold at `:847`); the consts are `in_ECX[0x5667]` (impulse) / `[0x565b]` (grav) / `[0x565e]`
  (terminal) — read them off the game when porting (don't curve-fit the asymmetry).

**NEXT (chip 3b/3c, in order):**
1. **PORT the jump** — extend `src/character.{c,h}` with `world_y` + `vvel` + the airborne integrator
   (impulse on a jump command, asymmetric gravity, ground clamp), host-tested vs the captured arc
   (the same "fit to RETAIL's captured bytes" discipline as chip 3a). RE the apex/fall branch +
   variable-height (hold C = cmd[2]=8) first so it is RE'd, not curve-fit; add `in_ECX[0x5667/
   0x565b/0x565e]` to the field spec and capture the consts.
2. **Capture + port the DASH** (run) — inject two direction ring presses within the double-tap window
   then hold (`ids:[4]` ×2 + the held RIGHT axis) → cmd[0]=5/6 → capture the run cap (> the walk's
   240) → port.
3. **The LIVE wire** — the chip-4 freeroam hand-off (dialogue chip 4 → the `entity+0x200` control
   transfer) gives `character_step` its first live caller in `game_actor_update` → Arche walks/jumps
   on screen, the chip-2 collision mover/probes get a live grounded actor → USER visual-verify.

**OPEN (USER):** butterfly chip-1 drift visual-verify still pending (trace-studio `intro-1` ~1580-1670).
Debt: PORT-DEBT(char-run-jump / char-input-autorepeat / char-walk-tuning / char-collision-mover),
PORT-DEBT(held-axis-array-b), PORT-DEBT(effect-color-variant).

## Where we were — ckpt 115

**PHASE-4 chip 3a — Arche's freeroam WALK is PORTED + FIELD-EXACT (host-tested vs the ckpt-114
ground-truth capture). New `src/character.{c,h}` + `tests/test_character.c` (4 tests). 958 host pass
(+4).**  A field-exact open-air reduction of the AI `0x478ba0` (held-axis → command) + the `0x442a70`
case-0x75 walk integrator — mirroring butterfly chip 1 exactly (host-tested vs the capture; the LIVE
payoff awaits the freeroam hand-off, chip 4, since the port can't reach freeroam yet).  Modules:
`src/character.{c,h}` (NEW), `tests/test_character.c` (NEW), `tests/{Makefile,test_main.c}` (wiring),
`tools/flow/retail_fields.json` (the durable `0x478ba0` annotation), `docs/port-debt.md` (4 rows).

**The WALK law (the bit-exact target = RETAIL's captured per-tick worldX, `runs/mover-caller`):**
- the horizontal velocity accumulator is **`body+0x28`** (signed), NOT `body+0x18` (= the VERTICAL /
  jump velocity, which reads 0 the whole flat walk — this reconciles the ckpt-113 "vel=0" note).
- **accelerate (held):** `vel += 1600/tick` toward the cap **24000** → dwx = vel/100 ramps **+16/tick**
  (16,32,48,…,240) to the +240 cap, via the 150-B clamp-ramp `0x445db0`.
- **brake (released):** `vel -= 800/tick` toward 0 → dwx ramps **−8/tick** (240,232,…,8,0) to a stop.
- **facing `body+0x2c`:** holds (1 right) through the entire brake-to-stop; flips 1↔3 only at **v==0**
  when the opposite direction is commanded (the integrator's `local_14` v==0 facing flip), then accels.
- **commit:** `worldX += vel/100` — a flat reduction of the collision-aware horizontal mover `0x54db10`
  (town street flat → no clamp).  The real body `0xe637b80`, committed by `0x485fc0+0x96e`→`0x442a70`,
  tracks `arche_body.wx` tick-for-tick (RIGHT 19200→44280, then the LEFT mirror to 28440).

**The port (`character_step(c, axis_held[4])`):** the AI reduction reads the held L/R axis → a latched
walk direction (a `CHAR_INPUT_REPEAT_DELAY`=3 warmup reproduces the capture's 2 idle ticks before
motion, then self-sustains — PORT-DEBT(char-input-autorepeat)); UP/DOWN (`cmd[3]`), run (`cmd[0]`=5/6),
and jump (`cmd[2]`/`[4]`) are read but deferred.  The apply reduction ramps the velocity, flips facing
at v==0, and commits worldX.  `test_character.c` asserts the accel ramp, the cap sustain, the brake to
a stop, and the LEFT-from-rest symmetry against the **embedded captured worldX bytes** (not law-derived
— a pass proves the port reproduces retail's bytes).

**The annotation step (compounds coverage):** `0x478ba0` got the durable `char_ai` entry in
`retail_fields.json` (held axis U/D/L/R via `*(this+0x158a4)+0x114..0x120`, `cmd[0]` `+0x14854`,
speed-mode `+0x158a0`, facing) — names the next flow_diff on the freeroam mover.  No live port
`CALL_TRACE_BEGIN(0x478ba0)` mirror yet: there is no live caller until the chip-4 hand-off wires
`character_step` into `game_actor_update`.

**NEXT (chip 3, in order):**
1. **chip-3b run/jump — INPUT RE'd (ckpt 115, engine-quirk #102) + the full MOVESET recorded (USER
   ground-truth), but the run/jump MOTION won't fire in the town freeroam (4 captures).** The action map
   + live scancodes (`runs/runjump-gt`, spec `tools/flow/freeroam_runjump_fields.json`), with the
   USER-CONFIRMED roles: producer `0x46a880` slot `+0x124`=`*(*0x8a6e80+0x574)`=**C `0x2e` = JUMP**
   (AI→cmd`[2]=8`, jump-BUFFERED), `+0x128`=`+0x558`=**X `0x2d` = ATTACK/interact** (→cmd`[4]=0xe`); Z =
   sword sheathe/unsheathe (ring); **dash = a direction DOUBLE-TAP, hold the 2nd** (cmd`[0]`=5/6 via the
   ring detector `0x479e70`/`0x479960`, window `*0x8a6e80+0xf8`). Full moveset (dash/slide/crouch/
   defensive-pose/door/sword-poses/directional-attacks/jump-buffering) in quirk #102. **BONUS: the
   captures re-confirmed the chip-3a WALK port byte-for-byte** (`hvel`=`body+0x28`=1600..24000).
   **THE BLOCKER (key finding, 4 captures):** every run/jump COMMAND gets SET (cmd`[2]=8` jump fires on C;
   the dash ring double-tap is injected) but produces **NO MOTION** — `wy`/`vvel` stay 52000/0 even on a
   clean C tap from a fully idle/grounded rest, and cmd`[0]` stays 2 (walk) on the dash. ⇒ the integrator
   GATES the dash/jump vertical response on a precondition the **flat town/inn cutaway doesn't satisfy** —
   strong hypothesis: **dash/jump are platforming mechanics that need a DUNGEON / platforming scene**, not
   the flat inn freeroam. **NEXT: either (a) drive retail to a platforming area (past the town, where
   jump/dash are active) and capture there, OR (b) RE the integrator's jump/dash response gate (which
   `0x442a70`/`0x478ba0` field disables vertical motion in the town) — then capture + port.** Don't
   guess-port motion that can't be observed.
2. **The LIVE wire** — the chip-4 freeroam hand-off (dialogue chip 4 → the control transfer
   `entity+0x200`) gives `character_step` its first live caller in `game_actor_update` → Arche walks on
   screen, the chip-2 collision mover/probes get a live grounded actor (clears char-collision-mover),
   and the USER can visual-verify on the feed.

**OPEN (USER):** butterfly chip-1 drift visual-verify still pending (trace-studio `intro-1` ~1580-1670).
Debt: PORT-DEBT(char-run-jump / char-input-autorepeat / char-walk-tuning / char-collision-mover) (this
chip), PORT-DEBT(held-axis-array-b), PORT-DEBT(effect-color-variant).

## Where we were — ckpt 114

**PHASE-4 chip 3 — Arche's FREEROAM MOVER is PINNED + the input→position architecture is fully
RE'd. Pure ground-truth (no port code). 954 host pass (unchanged).**  Method: with the ckpt-113
held-axis harness driving the walk, `--call-trace` the integrator `0x442a70` over the walk window
with arg/this field reads and filter to Arche (`in_ECX+0x1d4==0xc35a`; in_ECX IS the ~90 KB
entity).  Artifacts: `runs/mover-caller/` (`find_mover.py` + the `cap/.../call_trace.jsonl`),
field-spec `tools/flow/freeroam_mover_fields.json`.  Full writeup: **engine-quirk #101 final
bullet** (amended); plan `plans/movement-system.md` chip 3.

**The architecture (TWO layers, BOTH shared with the existing actor system — the big result):**
freeroam character movement mirrors the butterfly exactly, but on the CHARACTER band and with the
FULL integrator.
- **AI / intent: `FUN_00478ba0`** — the RNG-free character update (the SAME fn townsfolk use; the
  counterpart of the butterfly's `0x47b990`).  Called `0x478ba0(body)` with `ECX=entity`.  Reads
  the HELD AXIS at `*(entity+0x158a4)` (= the input manager) `+0x114`=UP `+0x118`=DOWN `+0x11c`=LEFT
  `+0x120`=RIGHT (quirk #41 confirmed) + action buttons (`+0x124/+0x128`) + the keybind config
  `*DAT_008a6e80`.  SAVES+CLEARS then rebuilds the **command block `entity+0x14854`** (8 ints):
  **LEFT→`[0]`=1 (walk)/5 (run), RIGHT→2/6, DOWN→`[3]=+0x14860`=10, UP→`[3]`=0xb**, jump/action
  →`[2]=+0x1485c`=7/9, `[4]=+0x14864`=0xe/0xf.  Walk-vs-run gated by a speed-mode `entity+0x158a0`
  (`<2`→thr 10, `==2`→thr 0x50) + the run modifier `*(*0x8a6e80+0x510)==2`.  Also runs a 4-step
  collision LOOK-AHEAD via `0x442a70` into TWO STACK scratch bodies (`0x1a0270`/`0x1a0504`, both
  `<0x400000` — the 488+488 "shadow" calls, NOT the commit).  Explicit `0xc35a` special-casing
  (`478ba0.c:337,463`).  Uses `GetTickCount` for TAP auto-repeat (`0x479ca0`/`0x479960`/`0x47a7f0`);
  HELD walking reads the axis flag directly → no wall-clock dependence (a determinism win).
- **APPLY / commit: `FUN_00485fc0+0x96e` → `FUN_00442a70`** — the EFFECT/apply band pass (the SAME
  one that integrates the butterflies, `0x46cd70:71`).  `485fc0.c:348`:
  `FUN_00442a70(in_ECX+0x5215, iVar6, iVar6, 0, 0)` where `in_ECX+0x5215` (int*) = byte `+0x14854`
  (the command block) and `iVar6=in_ECX[0x10]`=`*(entity+0x40)`=the REAL body — an IN-PLACE
  integrate (param_2==param_3==body), gated `local_2c==0`.  The ONLY fn that commits Arche's
  real-body (`0xe637b80`) position (244 calls; `body_wx==new_wx`, tracking the walk
  **19200→40800→44280→24120**, facing 1=right→3=left).

**The observed position law (the bit-exact target outline; nail it during the port):** vel
`body+0x18` = 0 the WHOLE walk ⇒ a **direct position write**, NOT velocity-integrated.  The per-tick
displacement accelerates from rest (Δwx ramps **+16/tick**: 64, 80, 96, … ) to a **~+240 cap**, then
decelerates to a stop on release; facing flips 1↔3 with the travel direction.

**This RECONCILES quirk #101 finding #3** (ckpt 112's "separate party-leader path, candidates
`0x405e80`/`0x406210`/`0x40c380`"): Arche's AI is indeed not `0x47b990` — it's `0x478ba0` — but the
APPLY is the shared band pass, so `0x46cd70` DOES reach her (as an active band actor), it just never
reads the party array `+0x4030`.  The candidate guesses are superseded.

**The invocation site (RESOLVED statically, same ckpt):** the `0x46cd70` band walk, the `0x1160`
band (32 slots).  `46cd70.c:38-60` pass 1 dispatches each active slot on `entity+0x200` (`==0` →
`0x478ba0` character AI, `==1` → `0x47b990` effect AI; override `iVar6==0 && entity+0x1f0!=0`→1);
pass 2 (`:66-76`) calls `0x485fc0` for every slot.  So Arche in freeroam is an ordinary `0x1160`-band
actor → the port already mirrors this band order (`game_actor_update`); chip 3 adds the `0x478ba0`
branch + the full apply.  **Polarity nuance:** quirk #101 #2 said control-transfer sets `+0x200=1`
yet `==1` is the EFFECT AI and Arche provably routes to `0x478ba0` — reconcile by reading
`entity+0x200`/`+0x1f0` for `0xc35a` at port time.

**NEXT (chip 3, in order):**
1. **RE the run/jump scancodes** (the `0x8a6e80` keybind defaults; jump is an action button at
   inputmgr `+0x124`/`+0x128`, cmd `[4]=0xe`) → extend the held-trace walk to capture **run + jump**
   per-tick.  The `freeroam_arche_fields.json` body spec already reads her independent of the mover →
   the bit-exact target for the whole moveset.
2. **PORT** — the `0x478ba0` character AI (held-axis→command block) + the FULL `0x442a70` integrator
   (the port has only the open-air butterfly reduction; chip 2's `collision_move_vertical` +
   probes get their first LIVE caller here) → validate "Arche walks + stops at terrain" field-exact.

**USER-CONFIRMED (ckpt 113):** the walk is verified on screen.  **OPEN (USER):** butterfly chip-1
drift visual-verify still pending (trace-studio `intro-1` ~1580-1670).  Debt unchanged:
PORT-DEBT(held-axis-array-b), PORT-DEBT(effect-color-variant).

## Where we were — ckpt 113

**PHASE-4 chip 3 — the HELD-AXIS INJECTION HARNESS is landed + LIVE-VALIDATED: Arche WALKS in
retail freeroam. The chip-3 ground-truth blocker (quirk #101 finding #4) is CLOSED. 954 host pass
(+8).**  Modules: `src/held_trace.{c,h}` (NEW) + `tests/test_held_trace.c`; `src/main.c`,
`src/input.h`, `src/title_scene.c` (port wiring + mislabel fix); `tools/frida/opensummoners-agent.js`,
`tools/frida_capture.py`, `tools/mem_watch.py` (`--held-trace`); engine-quirk #41 amended.
Live artifacts: `runs/freeroam-walk/` (`trace-nav.jsonl` + `walk-held.jsonl` + `cap/`).

**The gap it closed.**  The freeroam character mover reads the HELD-AXIS array (`input-mgr+0x114`),
NOT the discrete event ring — so the ring injection (`--input-trace`) drives menus + the dialogue
Z-advance but leaves the controllable leader IDLE when walking (the ckpt-112 trace-b ring ids 3/4
moved her 0 px).  This adds the level-injection mode the movement ground-truth was blocked on.

**The RE (ground truth, not curve-fit).**  The producer `0x46a880` (the per-frame global input
update) fills array A each frame from the DInput keyboard buffer via the leaf query `0x5ba520`
(= `keyboard_state[scancode] & 0x80`), one slot per key (`0x46a880:1497-1512`):
`+0x114=UP(0xc8) +0x118=DOWN(0xd0) +0x11c=LEFT(0xcb) +0x120=RIGHT(0xcd)`, action buttons at `[4..]`.
So array A is PER-DIRECTION held booleans — the input.h/`0x56aea0` "[0]=vertical/[1]=horizontal"
label was wrong (it's UP/DOWN; the title menu's two reads are both vertical-list auto-repeat).
The producer doesn't clear inline; a per-frame flush (`0x56a220`) handles release — which is exactly
why hooking the LEAF is the right call (see below).

- **Retail injection (`agent` + `frida_capture.py --held-trace`):** hook the leaf `0x5ba520` and
  force its return to `0x80` (pressed) for the injected scancodes; the real producer then fills
  `+0x114` exactly as a physical keypress.  Hooking the leaf (vs writing the array directly) is the
  decisive choice — it covers both the array-mediated mover and any direct-query consumer, survives
  the hidden window's loss of DInput focus (we never depend on GetDeviceState), and needs NO model
  of the engine's per-frame clear/produce path (release is automatic).  A read-back diagnostic hooks
  the producer `0x46a880` onLeave and emits the 4 slots (`{kind:'axis'}`) — proves the injection
  landed and gives the mover work a live axis signal.  `g_held_inject_enabled` gates it; the agent
  also threads it through the flip/sparkle/scene anchors.
- **Port replay (`held_trace.{c,h}`):** the LEVEL counterpart of `input_trace` (edge/ring).  Parses
  `{"frame":N,"keys":[scancode|name]}` (names up/down/left/right) and rebuilds `mgr->axis_held[0..3]`
  EVERY frame from the current held set (clear-then-set, mirroring the producer).  `main.c` calls it
  at all 4 drive replay sites after `input_trace_replay`; the only current port consumer is the title
  menu (`axis_held[0/1]`), the freeroam mover will read it when ported.  8 host tests
  (parse scancodes/names, malformed, level-persist, catch-up, guards).

**LIVE VALIDATION (`runs/freeroam-walk`, the clean contrast vs ckpt-112's ring run).**  Drove retail
to freeroam (the stripped Z-spam `trace-nav.jsonl`, ring 3/4 walk attempts removed so the held-axis
is the SOLE mover), then `walk-held.jsonl` = RIGHT held flips 4560-4760, idle, LEFT held 4820-5020.
The `freeroam_arche_fields.json` field-spec read `arche_body` (leader-chain, code `0xc35a`) per flip:
- RIGHT window: `wx 19200 → 41760` (Δ **+22560**, facing 1), walk anim `celfr` cycling.
- LEFT window: `wx 45472 → 25320` (Δ **−20152**, facing flips 1→**3**), decelerating to a stop after
  release (realistic accel/decel).  The `axis` read-back showed `R=1`/`L=1` in the freeroam array.
- vs the ckpt-112 RING run: `wx` static at 19200, `vel`=0 — she never moved.
Walk montage pushed to the feed (6 panels showing the position shift).  **Caveat to chase next:**
`vel` (body+0x18) reads **0** the whole walk while `wx` changes — so her horizontal speed is NOT in
body+0x18; the mover writes `wx` directly or holds velocity elsewhere (a lead for pinning the mover).

**NEXT (chip-3 ground-truth, now unblocked):**
1. **The wx WRITERS are PINNED (ckpt 113, `runs/mover-pin`, `--hw` watchpoint on her body `wx`,
   armed flip 4540).**  Two instructions wrote it over the walk (6577 traps):
   - **`0x442a70+0x2f` (5941, 90%)** — `FUN_00442a70`, the shared kinematic COMMIT/apply (the
     butterfly-known `0x485fc0`→`0x442a70`).  Its top (`442a70.c:48-58`) copies a source state
     `param_2` into the body `param_3`: `body+4 = param_2[1]` (wx, the trapped +0x2f write),
     `+8`=wy, `+0x18 = param_2[6]` (vel).  So `0x442a70` COMMITS a precomputed kinematic state;
     the MOVER is its **caller** (computes `param_2` from input).  vel(body+0x18)=0 all walk ⇒ her
     walk is a direct position update (`param_2[1]`=new wx), NOT velocity-integrated.
   - **`0x54ded0+0x5da` (636, 10%)** — `FUN_0054ded0` (1555 B, called by `0x54db10`), a TILE-GRID
     COLLISION fn (reads region C/D `in_ECX+0x2c103c`/`+0x2c1038`, `worldX/0xc80` cell index) — the
     collision clamp on her wx, a sibling of the chip-2-ported `0x54e990`.
   ⇒ the chip-3 MOVER = the caller of `0x442a70` that handles the party leader (`0xc35a`) and reads
   the held-axis.  **DO NEXT:** call-trace `0x442a70` with a `ret_va` field over the walk window
   (`--call-trace`, frames ~4560-4760) → the ret_va names the caller; filter to Arche's body.
   Candidate callers (xrefs of `0x442a70`): `0x43f880` (move-cmd FSM, butterfly-used), `0x4834f0`,
   `0x481ac0`, `0x441f50`, `0x4412d0`, `0x478ba0`, … — NOT the EFFECT band (`0x485fc0`, quirk #101).
   Then read that caller's held-axis read (`+0x114..0x120`) → the input→position law → the port.
2. **Capture walk/run/jump per-tick** (run = a modifier scancode held + dir; jump = an action button,
   scancode from the `0x8a6e80` keybind config — RE the default).  The `freeroam_arche` body spec
   already reads her independent of the mover → the bit-exact target.
3. **THEN port** — the party-leader update path + input read + the chip-2 mover/probes get their first
   LIVE caller (validate "Arche walks + stops at terrain" field-exact, then the slope resolver).

**USER-CONFIRMED (a):** the freeroam WALK is verified — "the trace is set up correctly and arche moves
the way you said" (Arche shifts right then left under held injection).  **OPEN (USER):** (b) butterfly
chip-1 drift visual-verify still pending (trace-studio `intro-1` ~1580-1670).  Debt:
PORT-DEBT(held-axis-array-b) (the port replay models array A only; array B at +0x140 is un-RE'd —
add when a consumer needs it), PORT-DEBT(effect-color-variant) (INN townsgirl wrong colour variant).

## Where we were — ckpt 112

**PHASE-4 chip 3 GROUND-TRUTH (USER chose "ground-truth freeroam first") — the HOUSE FREEROAM
is REACHED in retail and four chip-3-reshaping facts are pinned. NO port code (pure RE). 946
pass (unchanged).**  Retail ground truth: **engine-quirk #101**; artifacts: `runs/freeroam-gt/`
(`cap` = the dialogue→freeroam frame sweep + the hand-off field-spec; `capb` = the
controllable-Arche per-tick capture); throwaway field-specs `tools/flow/freeroam_handoff_fields.json`
+ `freeroam_arche_fields.json`; the drive traces `runs/freeroam-gt/trace-{retail,b}.jsonl`.

The method: drive retail past the whole town-arrival cutscene under `--seed-pin --lockstep
--no-turbo`, Z-spam (ring id `0x24`) every ~12 flips from game_enter (1420).  The ~15 dialogue
beats clear and control transfers to the player INSIDE the inn (the **"PLAYER!" prompt + HUD
fade-in at flip 4500 / sim-tick 1556**, settled ~4660; pushed to the feed).

The four findings (full detail in quirk #101):
1. **Freeroam needs NO map reload** — one `game_enter` (1420); the inn interior is the same
   scene (cutaway).  So the USER's "house freeroam" target is reachable by advancing dialogue.
2. **The party leader `room_state+0x200c` is PERSISTENT (Arche since new-game), NOT cutscene-set.**
   A per-Flip chain read `*(*(0x8a9b50+0x2784)+0x200c)` = a constant slot (entity code `0xc35a`)
   from game_enter through freeroam.  The transfer flips a **per-actor controllable flag**
   (`entity+0x200=1` via the setters `0x41e070`/`0x4c6830` + a `0x54e5c0` placement), not the
   leader.  ⇒ the chip-3 leader-render via `0x4997b0` is real but the leader is already Arche.
3. **Arche's freeroam mover is NOT `0x47b990`.**  In freeroam a `0x47b990` hook fired only for
   `0xc3dc`/`0xc440` (Father/Mother), never `0xc35a`; the band walk `0x46cd70` never touches the
   party.  Her freeroam update is a SEPARATE party-leader path (the `0xc35a` case in `0x47b990`
   is the CUTSCENE-actor behaviour — a correction to the chip-3 plan, which assumed `0x47b990`).
4. **Freeroam movement reads the HELD-AXIS array `input-mgr+0x114` (quirk #41), NOT the event
   ring.**  Z-advance worked (ring), but injecting direction ids 3/4 every flip left Arche fully
   idle (`wx`/`vel`/`facing` constant, only the idle-anim `celfr` cycling).

**NEXT (the chip-3 ground-truth, blocked on a harness gap):**
1. **Add a HELD-AXIS injection mode to the harness** (the current `--input-trace` only fills the
   discrete ring `0x43c110`; held movement needs `input-mgr+0x114` `array_A[0/1]` filled each
   frame).  Likely a new trace kind / agent hook writing the axis array (watch the DInput
   producer doesn't zero it on a hidden window).  This unblocks ALL movement ground-truth + the
   eventual port validation.
2. **Pin Arche's freeroam MOVER** — once she walks, `mem_watch` her body worldX writer (or hook
   the leader-body readers `0x405e80`/`0x406210`/`0x40c380` + the held-axis readers
   `0x43bca0`/`0x448cb0`/`0x44e730`/`0x4539b0`) → the function to port for chip 3.
3. **Capture her walk/run/jump per-tick** (the leader-chain body spec in
   `freeroam_arche_fields.json` already reads her body independent of the mover) → the bit-exact
   target → THEN port (party-leader update + input + the chip-2 mover/probes get their LIVE caller).

**OPEN (USER):** (a) verify the freeroam-reached result on the feed (flip 4500/4740). (b) the
butterfly chip-1 drift visual-verify is still pending (trace-studio `intro-1` ~1580-1670).
Incidental debt unchanged: PORT-DEBT(effect-color-variant) (INN townsgirl wrong colour variant).

## Where we were — ckpt 111

**PHASE-4 chip 2 (TILE COLLISION) — the READ-SIDE CORE is PORTED + host-tested. 946 pass
(+6).**  The chip RE-SCOPED on a discovery: the town's collision GRID is **already built** by
`map_decode.c` (the `0x587e00` town arms deposit region B class/slope + C slope-type + D flag
per cell, on the proven-1:1 render path) — so "port the grid" was moot.  Modules:
`src/collision.{c,h}` (NEW), `src/map_grid.{c,h}` (read accessors), `tests/test_collision.c`.

- **`collision.{c,h}` — `collision_move_vertical` = `FUN_0054e990`**, the VERTICAL tile-grid
  mover (gravity/ground/ceiling clamp).  FAITHFUL + PURE over (grid, body box, delta) — its
  decompiled `in_ECX` is the GRID base, not the actor.  Sweeps world-Y in ≤100 steps, scans the
  X-extent cells vs region-B class (10=solid wall, 1=slope-surface w/ the verbatim `d==0`
  contact predicate), clamps + writes the new world-Y.  Slopes resolve via a caller callback
  (region-B `+0x8` = the exe VA `0x5cc410`/`0x5cc430` height profiles) — town is flat so unused
  → PORT-DEBT(collision-slopes).
- **`map_grid` read accessors** — `map_grid_obj_record/_class/_slope` (region B `0x140030`) +
  `map_grid_flag` (region D `0x2c1040`).  `idx = col*0x80 + row` (col=worldX/0xc80, row=worldY/0xc80).
- **`test_collision.c` (6 tests, exact clamps hand-derived)** — drop→floor (out 15000, ret 0),
  open-air clear (20000, ret 1), off-axis wall (no block), ceiling upward (9600, ret 0),
  zero-delta no-op, slope-callback wired.

**DEFERRED → chip 3** (they need a live grounded actor + the probes are actor-entangled, and
the user accepted "host-tested only, live payoff at Arche"): the directional AI probes
`0x441ae0`/`0x47dbb0` (read region D flag + region C slope-type + `in_ECX` actor state); the
integrator generalization (wiring the mover into `0x442a70`/`0x485fc0` — the butterfly apply
STAYS the field-exact open-air reduction, NOT regressed); `0x4412d0`/`0x440e40` = ENTITY-vs-entity
collision (the `in_ECX+0x40` actor list, stride 0x294 — corrects the plan's "tile grid" label).

**NEXT — chip 3 (controllable Arche).**  The party-leader band `0x4997b0`
(`plans/party-character-system.md`) + DirectInput → the `0xc35a` case in `0x47b990` →
walk/run/jump physics — this gives the chip-2 mover + probes their first LIVE caller (validate
"Arche stops at terrain" field-exact, then port the probes + integrator wiring + the slope
resolver).  The milestone.

**OPEN (USER):** the butterfly chip-1 drift VISUAL-VERIFY artifact is pushed (trace-studio
`intro-1`, game_enter ~frame 1580-1670, + a feed montage) — field-exact per `runs/butterfly-fsm/
compare.py` but not yet USER-confirmed 1:1 on screen.  Incidental find logged this session:
PORT-DEBT(effect-color-variant) — the INN townsgirl renders the wrong colour variant (port
brunette/blue vs retail blonde/pink, ~tick 276; same class as the butterfly multicolor, on
generic townsfolk).

## Where we were — ckpt 110

**PHASE-4 chip 1 is DONE — the butterfly OPEN-AIR PATROL MOTION is PORTED + FIELD-EXACT.
The 4 town butterflies now drift left↔right 1:1 (heading + facing field-exact vs the
ckpt-109 capture); the per-tick RNG stream stays bit-exact.  940 pass (+1).**  Module:
`src/butterfly.{c,h}` (grown from the ckpt-98 RNG-consumer stub), `src/main.c`
(`game_actor_update`: the apply-wire + the validation emit), `src/actor_spawn.c`
(`butterfly_register` args).  The full map is `plans/movement-system.md`.

- **Ported — the open-air REDUCTION** of `0x47b990` (AI) / `0x43f880` (move cmd) /
  `0x485fc0`→`0x442a70` (apply), all capture-verified:
  - **bounds at register** — `butterfly_register(freq, spawn_wx, slot)` sets
    `b1(+0x14264)=spawn_wx+11200`, `b3(+0x14268)=spawn_wx−8000` (the ckpt-109 dead constants).
  - **the `0xe29a` HEADING FSM** — the 2 RNG draws/tick (wander range + the 10% flag) MOVED from
    the consume-stub into their real use: decrement cooldown `+0x14248`, then FLIP heading
    `+0x14244` (1↔3) toward the far bound when `cd==0` AND (`|wx−bound|<0xc81` OR the 10% roll).
    The `0x47dbb0` collision term is OMITTED (open air = clear → PORT-DEBT, chip 2).  Draw
    count/order UNCHANGED — the ckpt-99 settled-town stream does NOT regress.
  - **the horizontal APPLY** — `butterfly_step` ends with the integrator EVERY tick (both gate
    phases, mirroring the band's 2nd EFFECT pass): `world_x += hvel` THEN `hvel` ramps ±10/tick
    toward `cmd_dir*100` (the capture's **step-before-ramp**: tick 0 has hvel 0 so dwx is 0).
    `facing` follows the velocity sign.  `main.c` mirrors each butterfly's `world_x`/`facing`
    into its rendered EFFECT actor (linked by `effect_slot` at register).
- **Validated field-exact (sim-tick axis, `runs/butterfly-fsm/compare.py`):** the port emits a
  `CALL_TRACE_BEGIN(0x47b990)` per butterfly per tick; diffed vs the ckpt-109 capture.  HEADING
  **0 mismatches** for all 4 (every flip tick exact, e.g. bf0 [35,85,155,199,243]) ⇒ the LCG is
  byte-aligned through **tick 269** (beats ckpt-99's 0-248).  FACING ≤1/286.  worldX exact
  BETWEEN reversals (bf0→t37, bf3→t51); residual = a **≤170-unit ≈ ≤2px BOUNDED** transient at
  turn-arounds (the deferred flap-coupling; non-accumulating because the flips phase-lock it).
  Host `butterfly_motion` (new) + `butterfly_pertick` (unchanged).
- **DEFERRED (PORT-DEBT, all retire with the full integrator chip 2/3):**
  - `butterfly-flutter` — the VERTICAL flutter sawtooth (`body+0x18` vel + the `cmd_2` flap
    sub-FSM = `0x442a70` case-3 `:832-877` → worldY bob) + the flap/heading-reversal COUPLING
    (= the ≤2px worldX residual).  worldY currently holds the spawn value (butterflies glide flat).
  - `butterfly-bounds-writer` — the +11200/−8000 spawn-time derivation un-RE'd (values exact).
  - per-instance frame_base multicolor (the white variant) — part of `butterfly-wander`.
- **NEXT — chip 2 (TILE COLLISION).**  Port the grid (`DAT_008a9b50+0x1048`→`+0x2c1030` row
  widths / `+0x2c1040` tile flags; index `worldX/0xc80`), the swept probe `0x4412d0`/`0x440e40`,
  and the directional probes `0x441ae0`/`0x47dbb0` — so grounded actors stop clipping terrain
  (prereq for Arche, chip 3 = the party band `0x4997b0` + DirectInput + the `0xc35a` case).
  This also generalises the `0x442a70` integrator → clears `butterfly-flutter`.  Deferred intro
  chips unchanged: fountain-anchor 2× (RE +1600 vs +3200), sky-anchor, R8 grade.  **USER
  visual-verify of the drifting butterflies pending (a fresh trace-studio session).**

## Where we were — ckpt 106

**R6 (the establishing-REVEAL frontier) is RESOLVED — the band renders `differ_px==0` at
every stamp-equal tick 2..32 on the recaptured intro-1; the whole remaining reveal-window
residual is the FOUNTAIN box alone (R7; smoke contributes 0).  939 pass.**
Retail ground truth: engine-quirk #100; the resolution writeup: parity-ledger R6.

- **What it was (two stacked causes — each fix alone made pixels WORSE, which is why
  ckpt 105 mis-read it):**
  1. The port GRADED the mask cels.  Retail binds res `0x458`/`0x583` through the plain
     getter `FUN_004184a0(0)` (`0x48e920:37/66`) — ungraded, quirk-#96 family.  Fixed:
     `SCENE_FADE_ALPHA_SLOT`(40)/`LETTERBOX_BANK_SLOT`(41) joined the grade skip-list
     (main.c).
  2. The ckpt-105b `hold>=2` fence held the reveal one tick behind its stamps.  Retail's
     tick-stamped frame presents the POST-update grid (mask-level extraction:
     `s5(a)==a` exactly).  The fence's original dt-scan justification was computed over
     graded cels — biased exactly one tick.  Fence REMOVED.
- **Hard proof artifacts:** `runs/r6-grid` (retail per-row grid dump, the new `0x499ab0`
  `sf_*`/`ovl_*`/`r40..r80` chain fields) + `runs/r6-grid-port` (the port mirror) — the
  grids are bit-identical 41 rows × 31 ticks at every pre-step boundary; the per-row
  staircase is `timer(u,d)=100u+50−50d`.  The mask sheet (`runs/extract/sotesd/type=DATA/
  1112.bin`) is a pure linear ramp: visual cell v ⇒ `gray5==v`; the group-E weight-1000
  mode-2 LUT is exact `clamp(d−s)` — so effective `s5` IS the frame index, which made the
  cel-content extraction decisive.
- **Bycatch (quirk #100):** the fade-grid object's +0x20/+0x24/+0x28 = an overlay
  AUDIO-fade ramp (town entry: mode 2, speed 500 → level 0→1000 at +10/tick over 100
  ticks), consumed by ~12 DirectSound position updaters (`(level−1000)*3` → vol clamp
  [−10000,0] → the `0x5bb870/80/90` vtable thunks = DSound volume/pan/frequency).  NOT a
  video term.  Needed when the town's ambient sound is ported.
- **Tooling hardening:** `tools/trace_studio/drive/port.py` must give the port child a
  PIPE stdout — WSL interop's exec of a Windows binary fails its vsock handshake
  (`UtilAcceptVsock accept4 errno 110`) when stdout is a regular FILE (this also wiped
  the intro-1 port side once before the diagnosis; recaptured fine after).
- **NEXT (in order):** (a) **R7** — the dual blit trace: the RETAIL half is captured
  (`runs/r7-blits-retail`, flips 1489-1496 ≈ ticks 29-32); run the PORT half over the
  same ticks (`--call-trace`, flips ~1174-1186), then `render_diff` → per-particle
  (res,frame,dst) deltas; suspects PORT-DEBT(fountain-anchor) +1245 vs the emitter age
  origin.  (b) **R8** — the typewriter row-close grade (the `0x48da70` thisderef
  field-spec recipe in the ckpt-105 NEXT below).  (c) dialogue chip 4 (Z-advance +
  script table + arrow re-probe).  (d) the probe-flag removal chip.  (e) when intro-1 is
  exhausted: a NEW deeper working trace (advance past the dialogue into the house
  freeroam scene) — USER directive 2026-06-10.

## Where we were — ckpt 105

**The SIM-TICK AXIS is in the trace studio end-to-end, the whole intro-1 worklist is
attributed at forced tick-equality, and the 3 measured trigger constants are recalibrated
onto the tick axis (banner/pan/dialogue all `differ_px==0` at tick-equal).  939 pass.**
Retail ground truth: engine-quirk #99; residuals: parity-ledger #13 + R6/R7/R8; debts
updated: banner-trigger / dialogue-trigger / ingame-camera-pan / dialogue-pause-grades /
scene-fade-window / fountain-anchor.

- **Instrumentation (the timestep-determinism rule, now tooled).**  Port: `g_sim_tick_count`
  (main.c) counts easer steps exactly like the retail agent's `0x43d1d0` hook; `--capture-all`
  BMPs are named `port_frame_<flip>_t<tick>.bmp`.  Studio: `index_frames` parses both sides'
  `_t` names, pair rows carry `port_tick`, `state.jsonl` gets `port.sim_tick`, the viewer
  shows `sim_tick port N / retail M` (red on mismatch), worklist rows print both ticks.
  **Chase any mark at FORCED tick-equality (pull both `_t` maps, probe dt around 0) — the
  pairing's pixel-driven drift wanders ±3 ticks through content-quiet stretches.**
- **The intro-1 marks, attributed:**
  - @1177 **NPC anim** — differ_px=0 at every sampled tick-equal pair → pairing phase, CLOSED.
  - @218 **lizsoft fade** — R3 boot stretching (retail's wall-clock title accumulator renders
    each value 2-4×; the port machine is decompile-exact: +0x14/−0x14, 0x32 hold); the logo is
    differ_px=0 at MATCHED fade level across its whole lifecycle (fade-in/hold/out).  CLOSED.
  - @2159 **banner fade-out** — NOT sampling noise: the banner ran 2 ticks early end-to-end
    (trigger offset).  CLOSED by the 78→82 recalibration (both edges differ_px==0 now).
  - @1122 **reveal** — REAL: the iris frontier runs ~1 tick ahead at tick-equal; the divergent
    region = the whole ~20-row/side fading band (cells live 10 ticks × 4 rows/tick).  R6 OPEN.
  - @1177 **fountain** — REAL: the particle ensemble differs ~2-4k px at EVERY dt∈[−8,+8] →
    a position/age model offset (anchor/age), not a tick origin.  R7 OPEN.
  - @2463 **text reveal** — REAL: the row-1→row-2 pause structure differs (retail {5,14,5} vs
    the fitted {1,5,16}, net −3) on top of the (now fixed) 8-tick arm lag.  **The ckpt-104
    "zero-mean oscillation / phase pillar" verdict is RETRACTED** — it read the flip-axis
    pairing, whose drift absorbed the constant lag.  R8 OPEN.
  - @1627 butterflies — unchanged known debt (butterfly-wander, movement FSM Phase 4).
- **Trigger recalibrations (all tick-axis, quirk #99):** `BANNER_ARM_FRAMES` 78→**82** (first
  alpha step t42 both sides; the 2.5-tick alpha-ramp plateaus made dt-probes under-read the
  error — calibrate fades off per-present VALUE sequences), `GAME_CAMERA_HOLD_FRAMES`
  184→**182** (cmd t92, first move t93; tick-equal pan residual = fountain/smoke/butterfly
  clusters only, localized + named), `DIALOGUE_ARM_FRAMES` 1298→**1282** (arm t642 → first
  visible change t645 = retail's; the pop/portrait change SEQUENCE pixel-identical at Δ=0).
- **R6 phase A LANDED (same ckpt):** the reveal's first `scene_fade_step` is fenced one tick
  (`main.c` hold>=2, modelling the beat-runner arm-request latency) — the dt minimum moved
  +1→0 and the @1122 mark box improved at tick-equal (t9 10318→7251, t13 7369→6579); banner/
  pan/dialogue/NPC unaffected.  **Phase B OPEN (the real remaining reveal residual):** at
  matched tick the frontier band's per-row LEVELS mismatch with structure — every 4th row
  equal, interior rows the PORT ~1 ramp index clearer, clear-edge rows (53/67 at t13) RETAIL
  already cleared (timer≥1000 vs the port's 900) — opposite directions ⇒ not a uniform shift,
  even though the setter `0x49a890`, the `0x499ab0` aging loop, and the `0x48e920` render all
  read byte-identical to the port.  Per-row ratio table in parity-ledger R6.
- **NEXT (in order):** (a) **R6-B** — dump res `0x458`'s per-index luminance through the
  group-E descriptor (render each index standalone in the port), invert BOTH sides' measured
  row-ratio maps to integer (marked-tick, stagger) per row, solve for the model delta
  (suspects: the timer cap/clear-vs-render order, a +50 half-step in fresh-mark aging).
  (b) **R7** — dual blit trace at ONE matched tick (`render_diff` lens) → per-particle
  (res,frame,dst) deltas; suspects: fountain-anchor (+1245), smoke anchor, population age.
  (c) **R8** — RE the typewriter char-class→grade map, esp. the ROW-CLOSE grade
  (`0x439690:499-505` slots; the stepper that advances `+0x170[4]/[6]` is NOT yet located —
  413b70/419eb0/40df40/4123a0 are all SETTERS; next: a `thisderef` field-spec on `0x48da70`'s
  node reading `+0x170` [4]/[6]/[0x18] per flip pins the grade table empirically without the
  static hunt).  (d) dialogue chip 4 cont. (Z-advance + ~15-line script table + module-aware
  arrow-bank re-probe) + the probe-flag removal chip (ckpt-103 leftover).

## Where we were — ckpt 104

**The in-game DIALOGUE BUBBLE is ported + bit-exact in-window (`src/dialogue.{c,h}` + `main.c`
game_render_dialogue) — the intro-1 worklist's big mark (@2429 pop-in) is CLOSED.  939 pass (+7).**
Model: engine-quirk #97 (+#98 formats); parity-ledger #12; plan `plans/dialogue-cutscene.md`
(chips 3 + the typewriter half of 4 done; Z-advance/full-script/beat-runner remain).

- **What renders (all differ_px==0 at paired drift on intro-1):** the 9-slice bubble (res
  `0x456`, slot 50) with the `+0x1c==1` SCALE pop-in (+50/update, content gated till 1000);
  the speaker-anchored bubble-TAIL pair (box-bank frames 9/10 at clamp(speaker)−16, box
  bottom — NOT the box left edge, a first-cut misread the studio loop caught as an 851-px
  residual); the name tab (res `0x44a` slot 52, f0); the name (white + `0x455f7b`, 3-pass);
  the portrait bust (res `0x7ef` slot 663, 24bpp keyed, 1:1 at (150,76)) with the `0x49c910`
  cross-fade — **the incoming cel SNAPS opaque at fade 500** (round-1 catch: a hold-at-19
  model lags retail); typewriter body rows (Courier 7×18, `0x3e537d`/`0xa8b9cc`, 310/168
  +28/row, 5 updates/char, comma 3i, space 1, row close +i — fitted).  Armed at
  game_enter+1298 (PORT-DEBT dialogue-trigger); text/name from the user's exe by VA
  (`0x86d58c`/`0x6b6f80`).
- **The USER's mark @2463 ("retail a couple frames ahead on the text reveal") = the phase
  pillar, zero-mean (measured):** reveal boundaries oscillate ±2 flips around the sticky
  pairing drift (retail ahead 2462/2472/2544, simultaneous 2516-2580, port ahead
  2558/2568/2590) — retail's lockstep tick-coalescing (the R5 mechanism).  Cadence 1:1.
- **Format facts (quirk #98):** 24bpp blobs = plain BMP at +0x458+off, palette slot =
  uninitialized XP packer memory; the screen = sheet through ONE RGB565 round trip.  res
  1000 in sotesd = a parallax mountain (the probe's "arrow res 1000" is a quirk-#92
  collision; module unresolved → PORT-DEBT dialogue-arrow-art, benign in-window because the
  arrow HIDES during typing).  UI sheets decode UNGRADED → grade skip-list += slots 50/52.
- **NEXT:** (a) the remaining intro-1 marks — the @1122 reveal-phase + @1177 fountain/NPC-anim
  trio (suspect one sim-tick-origin cause; diff at matched sim_tick per the timestep memory),
  @218 lizsoft fade (boot phase), @2159 banner fade-out check; (b) dialogue chip 4 cont.:
  Z-advance + the ~15-line script table + a module-aware arrow-bank re-probe; (c) the
  probe-flag removal chip.

## Where we were — ckpt 103

**The TRACE STUDIO is built + live (`tools/trace_studio.py`, `docs/trace-studio.md`) — the
review loop is now: capture both sides → the USER scrubs port|retail|diff in the browser
(:8779) and drops divergence MARKS → `worklist.md` → Claude chases each with
render_diff/flow_diff → `recapture --only port` → re-check.  First live session `intro-1`
(2598 paired frames, whole intro).  36 tool checks.**

- **Live right now:** `serve --session intro-1` is running; the USER was pinged to review +
  mark.  NEXT SESSION: read `runs/trace-studio/intro-1/worklist.md` + `edits.jsonl` FIRST —
  the USER's marks are the work queue.  The known big town gap is the ckpt-102 dialogue-box
  chip (below); butterflies frozen + pan offsets are the other expected reds.
- **Studio facts:** sessions in `runs/trace-studio/<name>/`; pairing is anchor-segmented
  (boot/subtitle/newgame/prologue/game_enter) with sticky ±8 drift; ordinal-named frames
  (same `frame_<k>.png` = same moment all panels); `pair` re-analyses without re-driving;
  the prologue locks at −7 drift (192/290 bit-exact) — a clean segment LOCKS, a
  content-divergent one hunts.  boot/title redness = R3 (documented); pre-game_enter
  anchor-rng DESYNC = quirk #77 (this nav skips the title-sparkle pin; town re-pins).
- **Footguns (all hit live, all now handled/documented):** agent-side capture emit ceiling
  (else device.kill starves behind the frame firehose); leftover sotes.unpacked.exe must be
  killed through FRIDA (taskkill → Access-denied, elevated parent) and is pre-killed before
  each drive; WSL-interop vsock `accept4 failed 110` when launching the port exe from
  detached/sandboxed contexts — run captures from an interactive shell.
- **R5 (USER-observed pan spikes): RESOLVED by measurement** (phase correlation over the
  intro-1 pan): pan LOGIC identical both sides (same ease histogram −1×11/−2×9/−3×~135,
  same 434px total, step-for-step timing); residual = retail occasionally coalescing 2
  sim ticks into one present even under lockstep (3×/pan; pervasive live = the spikes).
  Phase pillar, zero logic divergence; flip-level replication would ride the planned
  beat-runner (`0x439690`) pump port.  parity-ledger R5.
- **Tooling debt:** the frida_capture one-shot probe flags (cursor/fade/pace/textout/box/res/
  parallax/rand) are superseded by the field spec — mechanical removal chip queued
  (`tools/archive/README.md`).  Plan B1 (unified scenario-test) is superseded by
  `trace_studio capture`.

## Where we are — ckpt 102

**The in-game DIALOGUE BOX subsystem is fully RE'd, the legal story-text reader is built+tested,
and the box render is ground-truthed — the foundation for the town-intro dialogue → controllable
Arche (Phase 3→4). 932 pass (+5). 2 commits; no pixels yet.** Plan: `plans/dialogue-cutscene.md`;
RE writeup: `findings/in-game-intro.md` "The DIALOGUE BOX subsystem".

- **Architecture.** Linear cutscene coroutine: script `0x4d7d80` case `0x334be` (dispatched by
  `0x40c380`) configures a beat on the scene-controller, then `FUN_00439680`→ the blocking BEAT-RUNNER
  `0x439690` (pumps frames via `0x48c150`+`0x5b8fc0` flip + `0x5b1030` pump until the beat completes;
  returns 6 on Escape). Dialogue setup `0x49d6e0`: text→`+0x8a`, name (actor `+0x750`)→`+0x28a`,
  voice→`+0x2e8`, portrait id→`+0x84` (via face table `DAT_006b6568`), dirty `+0x78`, beat `+0x20=1`.
  Beat types (switch `:1128`): 1 dialogue (Z = `0x43b980`), 2 flag, 3 camera-at-target, 4 entity, 6
  timer `+0x57c`, 7 portrait-anim, 9 entity. **Object model (to confirm):** `0x439680` tail-calls with
  no stack arg ⇒ `0x439690`'s `param_1` and `in_ECX` are almost certainly the SAME `this` (one
  scene-controller holding beat + dialogue + widget state).
- **Render path — both primitives ALREADY ported.** `0x48c150` (the driver letterbox/scene_fade/banner
  hang off) → `0x48c820` (widget-tree walk, 873 B, unported) → `0x48cf80` (9-slice frame = `newgame_box.c`)
  + `0x48e200` (GDI text = `glyph_render.c`).
- **exe_strings (COMMITTED).** `src/exe_strings.{c,h}` (pure `pe_string_at`, PE32 VA→file-offset) +
  `exe_strings_win32.c` (`exe_data_string` maps the user's sotes.exe). Story text/names are content in
  the exe `.data` (line 1 @ VA `0x86d58c`); the Steam DRM leaves `.data` intact (byte-identical packed
  vs unpacked) so a raw read works — read at runtime, never embedded. 5 host tests incl. real-exe
  validation (VA `0x86d58c` → "Ahh, here we are at last!").
- **Ground truth captured** (`runs/dialogue-probe`/`dialogue-portrait`, PNG `frame_03100`): box frame
  res `0x456` (9-patch, frames 0–8, corners 32×32, node **408×112 @ (174,148)**, alpha fade-in); speaker
  NAME header ("Arche's Father", Courier New **7×18**, color `0x455dbb`, ~(410,139), via `0x40fa00`); **2**
  body rows (the `%n` split; Courier New 7×18, typewriter ~1 char/10 flips, main `0x3e537d` + outline
  `0xa8b9cc`, 3 TextOut passes); advance arrow res `0x3e8` (frames 0–3 base 20, bobbing, ~(542,240));
  **large portrait bust** res `0x7ef` (160×176, magenta key `0xff00ff`, slot `DAT_008a809c`) on the left.
  Box appears ~flip 2743 (game_enter 1432, after camera pan + banner hold). Town script ~15 lines:
  Father `0x5f5e1d3`, Arche `0x5f5e165`, Mother `0x5f5e1d4`, Sana `0x5f5e166`; voices `0x3eb`–`0x3f4`.
- **NEXT (the immediate chip — all inputs ready):** `src/dialogue.{c,h}` — register res `0x456` (box) +
  `0x7ef` (portrait); compose box (`newgame_box`) + name + 2 text rows (`glyph_render`) + portrait at the
  captured geometry; arm via a measured-constant trigger (PORT-DEBT, like banner-trigger) after the
  banner in `game_render`; verify `differ_px==0` vs `runs/dialogue-probe`. Watch: the text LAYOUT
  (word-wrap into 2 rows + the `%n` break = `0x4031c0`/`0x413ed0`) and the GDI name path (`0x40fa00`) —
  for chip 1 the captured static positions can anchor it; generalise in the typewriter chip. Then
  typewriter + Z-advance + full script; then the beat-runner driver (retires banner/camera-pan/letterbox
  measured-constant debts); then Phase 4 (party band `0x4997b0` + movement FSM `0x43f880`).

## Where we are — ckpt 101

**The "Town of Tonkiness" area-title BANNER is PORTED + BIT-EXACT (differ_px=0) — USER-confirmed
"banner looks good". 933 pass (+5).**  Module: `src/banner.{c,h}` (NEW) + `main.c` wiring.  Full RE:
ckpt 100 (engine-quirk #96, findings "The area-title BANNER"); the port: parity-ledger #11.

- **The producer (corrects the long-standing `0x5a00c0` attribution).**  The area card is `FUN_00494a60`
  (918 B), called 3× from the render driver `0x48c150:176-178` (slots `view+0x11c/0x120/0x124`, AFTER the
  `scene_fade` reveal grid); only slot0 is the title.  `0x5a00c0` is a SEPARATE thing — the scrolling
  story-text / dialogue overlay player (deferred).  mode 1 = the scroll SPRITE (res `0x449`, slot 53 /
  the `0x8a7714` bank) with the area name GDI-composed onto it, blit at (160,64).
- **The model.**  Animation = `0x499ab0`'s mode-1 phase machine (once/sim-tick = every 2 flips): phase 0
  compose → 1 fade-in (`alpha+=0x14`→1000) → 2 hold (`hold_ctr`→400) → 3 fade-out (`alpha-=0x14`→off).
  Render = `0x494a60` case 1: GDI text (Courier New h20 w10, white + `0x404040` 12× outline shadow,
  centred `x=160-(len*adv)/2`) composed onto the scroll cel via `zdd_object_get_dc`/`TextOutA`/
  `release_dc`, then keyed blit (alpha 1000) or alpha blit (ramp_b, fading).
- **Ported (`banner.{c,h}` pure + host-tested; `main.c` Win32 sink).**  `banner_arm`/`banner_step`/
  `banner_text_layout`/`banner_alpha_ramp_index` (pure) + `game_render_banner`/`banner_compose_text` +
  the arm at game_enter+78 + step/render in the sim-tick block after `scene_fade`.  All primitives
  already existed (GetDC, `glyph_row_draw`, `ar_make_font`, blits, `g_ramp_b`, slot 53 via
  `ar_register_palette_ramps`).
- **BIT-EXACT `differ_px=0/36720`** (whole banner region, camera-matched port 1300 ↔ retail
  `runs/banner-verify` 1614).  **The decisive fix:** the scroll sheet (slot 53) decodes UNGRADED —
  retail binds it via the plain getter `0x418470(0)` (no `0x417c40` grade), so skipping the in-game 8bpp
  palette grade for slot 53 made the parchment bit-exact (graded → ~10% too dark).
- **NEXT (open):** (a) the `0x5a00c0` dialogue box + scrolling story-text overlay; (b) the movement FSM
  `0x43f880` → butterflies drift + controllable Arche (Phase 4).  Residual debt: PORT-DEBT(banner-trigger)
  (arm +78 / text / hold = measured constants, real source = the scene script — same as camera-pan +
  letterbox), (banner-grade) (the slot-53 grade-skip is by index; faithful gate = the `0x417c40` getter),
  (banner-font-table) (only the font-6 length band).  Artifacts: `runs/banner-{probe,state,blits,verify}`,
  `tools/flow/banner_fields.json`.

## Where we are — ckpt 100

**The area-title BANNER is fully RE'd + ground-truthed (RE milestone, no port code — see ckpt 101 for the
port).**  Producer `FUN_00494a60`; the `0x5a00c0` attribution corrected.  engine-quirk #96; findings
"The area-title BANNER".  Ground truth: `runs/banner-probe` (PNGs/textout/res — the banner is a parchment
scroll res `0x449` 314×108 @ (160,64) with the GDI-composed area name, arms ~game_enter+78, holds the whole
intro), `runs/banner-state` (the 3-slot `0x494a60` field-spec → only slot0 enabled, the full alpha/phase
timeline), `runs/banner-blits` (the res-0x449 keyed blit).  New reusable field-spec `tools/flow/banner_fields.json`.

## Where we are — ckpt 99

**The SETTLED-town per-tick RNG stream is now bit-exact across the WHOLE window (not just the
REVEAL) — the four irregular ambient/event timers are ported, closing PORT-DEBT(fountain-rng-phase).
922 pass (+1).**  Module: `src/ambient.{c,h}` (NEW), `src/main.c` (`game_actor_update` band order),
`tools/flow/retail_fields.json` (the `0x5531b0`/`0x467380` timer-state field-spec entries + the new
`argfield` agent source).  Full RE: engine-quirk **#95** (amended with the corrected attribution);
`findings/in-game-intro.md` "The per-tick RNG stream" is the writeup home.

- **What the residual actually was.**  Not "all `0x5531b0`" (the ckpt-98 guess) but FOUR self-clocked
  timers, each a clean unit-decrement countdown (pinned by the seed-pinned timer-state capture
  `runs/ambient-timer` — direct `+0x5c`/`+0x20c` reads):
  - `0x11370` — `0x5531b0` ambient SOUND emitter (CHARACTER band): init `(rand*300)>>15`=33, fires tick **33** (+3).
  - wagon `0x1872d` — `0x54f980:932` idle-wander (CHARACTER): init `(rand*300)>>15`=134, fires tick **134** (+3).
  - `0x467380` — `0xe2a5` event timer (EFFECT band, via `0x442a70`): `+0x20c`=184 (spawn-set), fires tick **183** (+4).
  - `0x1136f` — `0x5531b0` ambient SOUND emitter (CHARACTER): init `(rand*300)>>15`=189, fires tick **189** (+3).
  The butterfly flit re-fire (tick 162, +7) was already modelled by `butterfly.c`.  **The census's
  earlier C-values (141/189/33) were off-by-one** — the `0x5bf505` `rng_state` field is the state
  *before* the draw (value = `rval(step(state))`); the `cd` reads are authoritative.
- **Ported (`ambient.{c,h}`).**  Four consume-to-advance timers in the proven `0x46cd70` walk order:
  EFFECT `butterfly_step → ambient_effect_step` (`0x467380`), then CHARACTER `fountain emit → sky emit
  → ambient_character_step` (`0x1136f`, then `0x11370`, then wagon — slot order; the 3 tick-0 inits
  must run in that order so each gets the right value).  Replaced the ckpt-98 blanket 3-draw consume.
- **Validated 3 ways:** offline LCG replay **0/297** vs the capture's `0x46cd70` checkpoints (the FULL
  298-tick window); **live port bit-exact ticks 0-248** (two frame-window `--call-trace` runs — the
  port's per-tick `0x46cd70` rng matches retail tick-for-tick through ALL FOUR fires 33/134/183/189,
  incl. the `ambient_effect_step` event timer at 183); host test `ambient_pertick`.
- **Debts moved:** fountain-rng-phase RETIRED (RNG stream bit-exact at the settled town).
  actor-protagonist-clip narrowed (the wagon's idle-wander RNG draws are ported; only the `0x43f880`
  motion remains).  NEW PORT-DEBT(ambient-event-cd): the `0x467380` cd-init is the seed-pinned 184
  (real source = the unported `0xe2a5` spawn arm `0x431cb0`).
- **NEXT (open):** (a) the `0x5a00c0` "Town of Tonkiness" banner + portrait/textbox overlay (the next
  clearly-visible town gap; high leverage — also gates the letterbox heights + camera-pan trigger);
  (b) the movement FSM `0x43f880` → the butterflies + wagon drift + controllable Arche (Phase 4).
  Artifacts: `runs/ambient-timer` (the timer-state ground truth), `runs/ambient-port` (the live port trace).

## Where we are — ckpt 98

**The town's PER-TICK RNG STREAM is ported + LIVE-bit-exact through the establishing-REVEAL
window — the long-standing "intro-cutscene RNG" sweep.  921 pass (+2).**  Modules:
`src/butterfly.{c,h}` (NEW), `src/actor_spawn.c` (cutscene-cast spawn draws + the butterfly
freq capture), `src/main.c` (`game_actor_update` order).  Full RE: engine-quirk #94 (the
room-load burst) + #95 (the per-tick model); `findings/in-game-intro.md` is the writeup home.

- **The sweep, in two chips:**
  1. **ckpt 97 — the room-load SPAWN BURST (engine-quirk #94).**  The first in-game frame draws a
     19-object EFFECT burst (15 MAP via `effect_from_map` = 171 draws + **4 SCRIPT cutscene cast** via
     `cutscene_cast` = 42 draws: Arche `0xc35a` 12 + 3×10), THEN the REVEAL iris-variant draw.  The port
     consumed only the 15 map → iris variant 2 (sweep); now `cutscene_cast` consumes its 42 → variant
     **0 (center-out)**, retail's value.  Only the COUNT matters (LCG state is value-independent).
     `0x4f5347`+214 draws = `0x9c2b551d` = retail's tick-0 state ⇒ **the spawn is byte-aligned.**
  2. **ckpt 98 — the PER-TICK stream (engine-quirk #95).**  `0x46cd70` walks the bands; only the 4
     BUTTERFLIES (`0x47b990`, EFFECT band, update-mode 1) + the fountain/sky emitters (`0x54f980`,
     CHARACTER band, already ported) draw RNG.  `src/butterfly.{c,h}`'s `butterfly_step` runs the
     `0xe29a` draw model once/sim-tick BEFORE the emitters (the gate `0x14232` + flit timer `0x14236` +
     heading/flag, each butterfly's `0xc874` captured at spawn).  COUNT model: `tick0=23`, then
     `6 + 8·[even] + 8·[N≡5 mod6]`.
- **Verified:** offline LCG replay reproduces retail's `0x46cd70` onEnter rng for ticks 0-33 (293/298
  overall; the 5 misses are the irregular `0x5531b0` ambient ×3 + the flit re-fire at 162 + 2 unknown);
  host tests `actor_spawn_cutscene_iris` + `butterfly_pertick`; **LIVE the port's per-tick rng matches
  retail tick-for-tick** (`0x9c2b551d, 0xb92fc6fa, 0x5c22a348, …`, ticks 0-11).
- **Debts moved:** scene-fade-rng-phase → `scene-fade-window` (variant drawn; only the black-load start
  offset left).  fountain-rng-phase narrowed to the irregular `0x5531b0` ambient.  butterfly-wander is
  no longer RNG-blocked — the DRIFT now waits on the movement FSM `0x43f880` (5.5 KB, Phase 4).
- **NEXT (open):** (a) RE + port the ambient timer `0x5531b0` (+ the 2 unknowns) → the SETTLED-town
  fountain/sky bit-exact (closes fountain-rng-phase); (b) the `0x5a00c0` banner/textbox overlay;
  (c) the movement FSM `0x43f880` → butterflies drift + controllable Arche (Phase 4).  Artifacts:
  `runs/rng-census-repin` (the per-tick ground truth).

## Where we are — ckpt 96

**The town BUTTERFLIES are PORTED — and the 4 `0xe29a` EFFECT objects were NEVER "wandering
villagers" (a mislabel that stood from ckpt 83 through 95).  USER-confirmed the retail
identification.  919 pass.**  Module: `src/actor_spawn.c` (+`BUTTERFLY_CLIP`, `0xe29a` in
`TOWN_EFFECT_DEFS`).  Full writeup: `findings/in-game-intro.md` "The town BUTTERFLIES" +
engine-quirk **#93**.

- **What they are.**  res `0x3fa` (bank `0x146`, sprite slot 313 = bank−13, 32×32, sotesd.dll
  DATA — already group3-registered, just unused), rendered by `0x493ba0` at layer 12, animated by
  clip `0x65ddf0` (decoded: base 0, 3 frames, dur 4, looping, delta {0,1,2} — a wing-flap).  4 in
  the map (EFFECT `0xe29a` ×4), 2 colour variants (yellow + white) selected by per-instance
  frame_base (0/4/8/12).  They wander (the 5 `0x427670`-case-2 draws #86 called "the wanderers"
  ARE the butterfly flit RNG).
- **How it was found (the methodology's live render trace at the SETTLED town, NOT the hold).**
  Drove retail to flips 2028/2138 (`--seed-pin --lockstep`, `trace-retail.jsonl`) with the render
  bands hooked: particle band rendered only the ported `0x18704`/`0x18708`, EFFECT band only
  townsfolk/cast → butterfly in neither.  A **blit trace** (`blt_keyed` res/dx/dy) found res `0x3fa`
  at the butterfly's screen pos; an **emit trace** (`0x492670` cel_res+ret_va) named producer
  `0x493ba0` at world positions matching the `0xe29a` census 1:1.  `runs/butterfly-{census,allbands,
  blits,emit}`.
- **Port.**  `0xe29a` added to `TOWN_EFFECT_DEFS` (bank `0x146`, dst 0/0, layer 12, facing 1, flip 4);
  new `BUTTERFLY_CLIP`; the spawn selects the per-code clip (butterfly vs IDLE_CLIP) BEFORE the
  `0x426ec0` phase draws — the draw COUNT is unchanged so the shared LCG stays aligned (no
  townsfolk-phase regression).  Previously `0xe29a` was excluded (draws consumed, not spawned).
- **Verified.**  919 host tests pass (`test_actor_spawn` updated).  Port blit trace: res `0x3fa`
  frames 0/1/2 emit on-screen as the camera pans (frame 1600 @dx 116/180, 1850 @dx 491/555) — two
  yellow butterflies flapping by the ARMS sign / flowers / dog, matching retail's placement.  Pushed
  to the feed.
- **PORT-DEBT(butterfly-wander):** per-instance direction/colour (the white variant via frame_base
  4/8/12) + the RNG wander drift (the 5 draws are consumed but the motion is not applied) — Phase 2,
  with the other scene-RNG residuals.  **NEXT — open options unchanged from ckpt 95** (whole-scene RNG
  phase parity / the `0x5a00c0` banner+textbox / arrival cutscene dynamics / controllable Arche); see
  ckpt 95 below.

## Where we are — ckpt 95

**The establishing REVEAL is PORTED — the center-out vertical iris that opens the town
from black.  USER: "the iris looks reasonable."  919 pass (+5).**  Module: `src/scene_fade.
{c,h}` (NEW).  Full writeup: `findings/in-game-intro.md` "The establishing REVEAL is PORTED".

- **The subsystem (a self-contained scene-transition fade-grid, the grid object
  `*(0x8a9b50+0x1040)`).**  A 10×120 grid of 64×4px cells over the screen; each cell
  `state 0 opaque → 1 fading → 2 clear`, `timer` 0..1000.
  - **render** `0x48e920` → `scene_fade_render` (after the letterbox, `0x48c150:175`): state 0 →
    opaque black res `0x583`; state 1 → alpha black res `0x458` frame[`0x1f-(timer<<5)/1000`];
    state 2 → skip.
  - **update** the INLINE loop `0x499ab0:125-177` (advance each fading cell's timer, 1×/sim-tick)
    + the iris **pattern setters** `0x49a890` (variant 0 center-out) / `0x49a740` (1 edges-in) /
    `0x49aae0`+`0x49aa00` (2 sweep) → `scene_fade_step`.
  - **arm** `0x439690:555-583` → `scene_fade_arm`: `mode=req+0x28`, `variant=(rand*3)>>15`,
    `speed=req+0x2c`, fill cells.  Live town: mode 1, speed 1000, variant 0 (W=10 H=120,
    `runs/reveal-grid`).
- **CORRECTS quirk #90 (important for future sessions):** `0x49af40` is **NOT** the grid update —
  it's the per-frame HUD/portrait/HP-bar animator (walks the party array `room+0x4030`, returns a
  counter).  The grid update is the `0x499ab0` inline loop + the `0x49a8xx` setters.  The
  −8px/sim-tick = mode-1's **2 rows/tick** × 4px, 1×/sim-tick (not `0x49af40` 2×).  Fixed in
  `engine-quirks.md` #90 + `retail_fields.json` (`0x49af40` → `hud_party_anim_update`).
- **Wired (`main.c`):** `enter_game` arms the grid after the spawn burst; `game_render` steps it
  once/sim-tick after the camera easer (the `0x439690:1124` order) + renders it after
  `letterbox_render`.  opaque sink = the letterbox cel (res `0x583`, slot 41); **alpha sink = the
  true `0x5bd550` composite** of res `0x458` (slot 40) frame[level] — the per-level GRAY MASK —
  through the descriptor **`g_pd_boot_group_e[19]`** (= `*(0x8a93b8)`, the group-E ramp `0x8a936c`
  [19]: weight 1000, mode-2 subtract-blend = darken the dest by the source).  The first keyed-blit
  cut drew the gray opaquely (USER: "white outside, black inside, no transparency"); disassembling
  `0x48e920` (the `0x5bd550` call at `0x48eaa9`) recovered the ECX descriptor Ghidra dropped
  (`runs/reveal-desc` confirmed it's unique vs ramp_a/b[19] → group-E).  VERIFIED on
  `port_frame_01160`: the blue town sky shows through, darkening to near-black across the edge.
- **Verified.**  Host test `test_scene_fade.c` (5 cases: arm fills the screen / center-out iris /
  edges-in / completes+done-latch / alpha ramp).  **Port blit trace:** black tiles 1490 → 650 → 320
  over frames 1118→1200, the center-out iris settling to the 64px letterbox by ~sim-tick 25 (=
  retail's 240→64 envelope).  Real composited capture pushed to the feed → **USER: "the iris looks
  reasonable."**  919 pass; ledger 204/199 (+5: the render `0x48e920` + the 4 iris pattern
  setters `0x49a890`/`0x49a740`/`0x49aae0`/`0x49aa00`; the partial `0x499ab0`/`0x439690` slices
  stay bare-VA).
- **BMP capture footgun (USER flagged "how is in-game capture broken?"):** it was never broken —
  I'd passed a WSL `--capture-dir /tmp/…` the Windows exe can't `fopen`.  The default (game dir
  after chdir) works; in-game capture works fine.  Added a `fopen`-failure hint in
  `capture_primary_to_bmp`.

- **NEXT — open options (the intro is visually complete + the reveal plays):**
  1. **Whole-scene RNG phase parity (Phase 2 proper)** — now the keystone for THREE pinned residuals:
     the reveal VARIANT (PORT-DEBT scene-fade-rng-phase — pinned to 0 until the post-spawn LCG phase
     matches), the townsfolk/Arche idle PHASES, and the fountain/particle positions.  All need the
     full per-tick + spawn LCG consumers ported in order.  Plus the reveal's load-window START offset
     (the port arms at `enter_game` with no black-load window).
  2. **The `0x5a00c0` "Town of Tonkiness" banner + portrait/textbox overlay** (PORT-DEBT
     ingame-nontile-layers) — the next clearly-visible missing town element (the golden-video review #89).
  3. **The arrival cutscene DYNAMICS (Phase 3):** the walk-in movement + dialogue runner `0x49d6e0`.
  4. **Controllable Arche (Phase 4):** the party band `0x4997b0` + the movement/physics FSM.
  Ground-truth artifacts: `runs/reveal-grid` (the `0x48e920` grid header live).

## Where we are — ckpt 94

**ARCHE RENDERS — the in-game intro cast is COMPLETE.  USER-confirmed on the live port
window: "everyone is rendering correctly now."  914 pass.**  The ckpt-93 gap ("all
characters except arche") is closed, and the heavy Phase-2 "party band" plan turned out
to be **unnecessary** for the arrival scene.

- **The shortcut (live census `runs/cutscene-cast`, the `0x493ba0` field spec).**  Arche
  (`0xc35a`) is drawn by the SAME `0x493ba0` EFFECT path as the rest of the cast, not a
  separate band: row0 bank `0x8b`, clip `0x62a8c8` (decoded byte-identical to the idle clip
  `0x6290e0`), world (41600, 45600), dst (−30,−24), facing 1, layer 13.  Her only blocker was
  **bank registration** (slot 126 = bank−13 had no sprite).
- **Her body banks `0x8b`–`0x8e` (slots 126–129) = EXE-embedded res `0x570`–`0x573`** — pinned
  by a field-spec **chain read** of the live retail slots (`g_ar_sprite_table[idx]` @
  `0x8a7640+idx*4` → slot → `+0x40`; `runs/arche-res`/`arche-params`, validated against known
  slots 168/222/338).  **Numeric collision (quirk #92):** those ids are `WAVE` sounds in
  sotesd.dll but `DATA` sprites in **sotes.exe**'s `.rsrc` — what derailed ckpt 90.  Loaded
  from the user's exe at runtime (`FindResourceA`), never embedded (USER directive).
- **Ported.**  `ar_register_party_exe_sprites` (asset_register.c) registers slots 126–129 with
  `settings = g_sotes_exe` (the module `ar_sprite_decode` reads from), called in `enter_game`
  after `load_town_scene` opens the exe.  `actor_spawn_cutscene_cast` (actor_spawn.c) gains an
  Arche row — handle `0x5f5e165`, `bank_override 0x8b` (her dramatist bank is 0 → `party_resolve_
  spawn` yields 0), her own world_y/dst_y.  She renders via `actor_render_static` (one keyed cel).
- **Verified.**  914 tests pass; build clean (both exes).  Port blit trace (settled frame 2200):
  res `0x570` frame 1 emits at screen (258, 304) = world (41600,45600) − settled cam + dst.  USER
  visually confirmed the live window.

- **NEXT — open options (no single forced next chip; the intro cast is visually complete):**
  1. **Whole-scene RNG phase parity (Phase 2 proper)** — the townsfolk + Arche idle PHASES and the
     fountain/particle positions are visually right but not yet `differ_px==0` (the per-tick LCG
     consumers `0x47b990`/`0x54f980` cases aren't ported, so the stream phase drifts).  This is the
     standing "validation pending" item back to ckpt 87.
  2. **The remaining golden-review gaps (quirk #89):** the establishing REVEAL fade-grid
     (`0x49af40`/`0x48e920`, quirk #90, unported), the `0x5a00c0` "Town of Tonkiness" banner +
     portrait/textbox overlay (PORT-DEBT ingame-nontile-layers), ground butterflies, the Start-Game
     scale transition.
  3. **The arrival cutscene DYNAMICS (Phase 3):** the walk-in movement + dialogue runner `0x49d6e0`
     + the `0x5a00c0` overlay — turns the frozen settled cast into the played-out arrival.
  4. **Controllable Arche (Phase 4):** the party band `0x4997b0` + the in-game movement/physics FSM
     (its own RE pass) — the gateway milestone.
  PORT-DEBT(cutscene-party-chars) narrowed to: party-band leader render + multi-part body
  `0x8c`–`0x8e` + walk-in roll-in + live-actor handle registry (dialogue).

## Where we are — ckpt 93

**The DRAMATIST RESOLVE + the arrival-cast spawn are PORTED — Arche's Mother (`0xc440` bank
`0xb5`) now renders her own sheet, and the frozen cast snapshot is retired.  USER-confirmed:
"all characters except for arche are there and positioned correctly."  914 pass (+3).**
Builds on the ckpt-92 RE (the dramatist table proof).  Module: `src/party.{c,h}` (NEW).

- **`src/party.{c,h}` (NEW, pure + host-tested).**  Ports the static "Get Dramatist Info"
  table `DAT_006b6ea8` (79 rows `{handle, code, bank}` — NUMERIC facts only; the names stay
  in `docs/proofs/dramatist-table.md` + the dump tool, NOT embedded as story content) +:
  - **`party_resolve_spawn(handle, code_in, facing_sel, &code, &bank)`** = `0x41f200:54-69`.
    The spawn path (`0x41f0e0`) passes the activator's param_4 = 3, so the row's code overrides
    only when `code_in==0`; the row's bank (`+0x30`) is the OVERRIDE (sVar17); if it is 0, fall
    to the archetype default.
  - **`party_archetype_default_bank(code, facing_sel)`** = the per-case `if (sVar17==0) sVar17
    = <facing default>` arm, the RE'd subset read off the decompile (`0xc3dc`/`0xc3dd`/`0xc3e6`/
    `0xc3eb`/`0xc3f0`-`0xc3f3`/`0xc440`/`0xc441`).  PORT-DEBT(dramatist-archetype-defaults).
- **`actor_spawn_cutscene_cast` rewritten** (`actor_spawn.c`).  Replaces the frozen
  `CUTSCENE_CAST_DEFS` identity snapshot with the family's RE'd `0x41f0e0` spawn params
  (`{handle, code_in, facing, facing_sel, x_off}`), resolving each through `party_resolve_spawn`:
  **Dr. Barnard** (handle 0, code `0xc3f0`, facing_sel 0 → base bank `0xeb`), **Arche's Father**
  (handle `0x5f5e1d3` → code `0xc3dc`, bank `0xe3`), **Arche's Mother** (handle `0x5f5e1d4` →
  code `0xc440`, OVERRIDE bank `0xb5`).  Mom's `0xb5` is registered in group3 (idx 168 =
  `0xb5`-13; the port resolves bank→`g_ar_sprite_slots[bank-13]`), so she renders — the fix.
  Positions = `CUTSCENE_WAGON_ANCHOR_X` (41600, the wagon's settled anchor) + the RE'd
  anchor-relative offsets {Barnard +25600, Father +8000, Mother −3200} → census {67200, 49600,
  38400} exactly.  Arche is dropped from the EFFECT cast (she is the party LEADER, not an
  `0x41f0e0` spawn).
- **The ONE remaining gap: Arche the GIRL** (`0xc35a`, dramatist bank 0).  She is the party
  LEADER (party band `0x4997b0`), her body banks `0x8b`–`0x8e` (idx 126-129) are UNREGISTERED
  (loaded by the unported new-game party sprite-load), her clip is `0x62a8c8`, render via the
  multi-part `0x493ba0` arm.  All four would cull today; deferred to Phase 2.
- **Verified.**  Host test `test_party.c` (3 cases: dramatist find / archetype defaults / the
  arrival cast resolves to the expected (code, bank), incl. Mom `0xb5` ≠ map `0xa6` and Arche →
  bank 0/return 0).  914 pass.  Settled-town port|retail side-by-side pushed to the feed →
  **USER-confirmed** the cast (Guard + Mom + Father + horses + Barnard) is present + positioned
  1:1; only Arche the girl is absent.  Ledger 199/194 unchanged (party.c uses bare-VA refs for
  the `0x41f200`/`0x426d70` slices, not `FUN_` forms).

- **NEXT — Phase 2: the party band + Arche.**  Port `0x4997b0` (the 8-slot + leader walk, the
  `slot+0x9f4`→entity→`+0x40` indirect render-state) + RE the new-game per-character sprite-load
  (where banks `0x8b`–`0x8e` get their resources → `ar_sprite_slot_register`) + the rich
  `0x493ba0` arms (shadow `0x4917b0`, color-remap, multi-part).  This renders Arche (gateway to
  the controllable leader).  First sub-step likely needs a live capture (hook the party spawn +
  `0x556eb0`) to pin Arche's exact sprite RESOURCE per bank.  Plan: `docs/plans/party-character-system.md`.

## Where we are — ckpt 92

**The DRAMATIST TABLE `DAT_006b6ea8` is RE'd — the arrival cast is NAMED from ground truth,
correcting ckpt-91b's tangled identities.  RE milestone (no port code yet, 911 pass).**
Proof: `docs/proofs/dramatist-table.md` (`tools/dump_dramatist_table.py`); full writeup
`findings/in-game-intro.md` "THE DRAMATIST TABLE (ckpt 92)".

- **The mechanism.** Character identity is a 32-bit **handle** → the "Get Dramatist Info"
  table `DAT_006b6ea8` (rows `{handle@+0, code@+4, name[0x28]@+8, bank@+0x30}`).
  `FUN_0041f200:54-69`: spawned with a handle + code 0, it looks the handle up, sets the
  actor's effective **code** `+0x1d4` = the row's code (the ARCHETYPE / sprite-switch
  selector), and uses the row's **bank** as `sVar17` to override the archetype's default
  sheet (`FUN_00426d70(0, sVar17, 0)`). **code = archetype, bank = the sheet.**
- **The arrival family** (cutscene `0x4d7d80:334be` spawns by handle, anchor `0x65`):
  `0x5f5e1d3`→`0xc3dc` bank `0xe3` = **Arche's Father** (renders ✓); `0x5f5e1d4`→`0xc440`
  bank `0xb5` = **Arche's Mother**; `0xc3f0` bank `0xeb` = **Dr. Barnard** (renders ✓);
  `0xc3e6` = **Guard**; **`0x5f5e165`→`0xc35a` = Arche** (clip `0x62a8c8`, banks
  `0x8b`/`0x8c`/`0x8d`), the persistent party LEADER created at new-game, referenced (not
  spawned) by the cutscene dialogue.
- **The corrected gap (vs 91b).** Only **Arche (`0xc35a`)** culls (sprite unregistered), and
  **Mom's real sheet (`0xc440` bank `0xb5`)** is mis-rendered — the port spawns the generic
  *map* `0xc440` bank `0xa6` townswoman (the `case 0xc440`="Woman" facing-1 default) instead.
  **Dad (`0xc3dc`) + Dr. Barnard already render.** `0xc3f0` is Dr. Barnard, NOT "the woman"
  (the `CUTSCENE_CAST_DEFS` decode-bug comment was the retracted ckpt-91 error — now fixed).
- **NEXT — Phase 1 port** (new `src/party.{c,h}` or extend `actor_spawn`):
  1. Port `DAT_006b6ea8` + the `0x41f200:54-69` handle→code/bank resolution + the archetype
     cases `0xc440` (Woman, default `0xa6`/override `0xb5`), `0xc3dc` (man, `0xe3`), `0xc35a`
     (Arche, banks `0x8b`–`0x8e` + clip `0x62a8c8`).
  2. Spawn the cutscene family by handle so **Mother → bank `0xb5`** (retire the frozen
     `CUTSCENE_CAST_DEFS` snapshot; PORT-DEBT cutscene-party-chars).
  3. Register **Arche's banks `0x8b`–`0x8e`** (`ar_sprite_slot_register`) + render her via the
     `0x493ba0` multi-part arm (party leader → gateway to controllable Arche).
  - The dramatist/handle **registry** (`0x556eb0` resolve / `0x555f00` add-remove over
     `DAT_008a9b50+0x2790`) is the *live-actor* lookup (distinct from the static def table):
     `0x556eb0` scans the array for an actor whose `+0x1d8` == handle.  The cutscene dialogue
     uses it (resolve a live speaker); the static def (`DAT_006b6ea8`) is used at spawn.
  - EXE-embedded sheets (if any party bank lives in `sotes.exe` `.rsrc`) → runtime extract /
     `%APPDATA%` cache, never embedded (USER directive).

## Where we are — ckpt 91b

**The PARTY-character system is SCOPED, PLANNED (USER-approved), and the arrival cast is
GROUND-TRUTHED + USER-confirmed.  There is NO decode bug — the woman (Arche's mom) + Arche
are MISSING party characters; the port never loads their sheets.  No port code ported yet
(RE + planning checkpoint, 911 pass unchanged).**  Plan: `docs/plans/party-character-system.md`.
Full RE: `findings/in-game-intro.md` "DEFINITIVE (ckpt 91b)" (supersedes "CORRECTION (ckpt 91)",
whose `bank=idx+13` stands but whose "woman=`0xc3f0`/decode-bug" was a cross-run-flip artifact).

- **`bank = registration_idx + 13`** — PROVEN (the bit-exact tree: bank `0x15f`=351, res
  `0x481`@idx 338).  Corrected the ckpt-90 `bank=idx` reads.
- **The cast (USER-confirmed, settled-state aligned):** `0xc3f0` `0xeb`→res `0x477` = **the MAN
  right of the horses — the port renders him CORRECTLY**; `0xc3dc`→res `0x473` etc. = townsmen;
  **`0xc35a` `0x8b`→idx 126 (UNREGISTERED → CULLS) = center, where Arche + the woman (mom)
  stand** (ckpt-90 was right).  **NO decode bug:** sotesd.dll res `0x477` is the man (port
  in-game dump + offline `lizsoft_sprite` decode agree); it's the only sprite source (sotesp.dll
  has no res 1143; the EXE's res 1143 is `MPED2DT` map data, not a sprite).
- **So the woman + Arche are missing because the party/character system (dramatist registry +
  per-character sprite loading + party band) is unported.**  `0xc35a` is the keyed party actor;
  Arche's richer multi-part render is the other half.
- **NEXT = execute the plan, Phase 1:** the dramatist/handle registry (`DAT_008a9b50+0x2790`,
  add `0x555f00`, resolve `0x556eb0`) + per-character creation & sprite loading at new-game
  (RE the character→resource→bank map + `ar_sprite_slot_register` per member).  First sub-step:
  a clean SINGLE-run retail capture (hook the party spawn + `0x556eb0`) to pin the exact
  `0xc35a`-vs-Arche split + each one's true sprite source (bank/resource/**module**).
- **EXE-embedded sheets (USER directive):** if a needed sheet lives in `sotes.exe`'s `.rsrc`
  (retail uses `FindResourceA(NULL,…)` for banks `0x570`-`0x572`), the port must NOT embed it —
  load from the user's packed `sotes.exe` at runtime (`LoadLibraryEx`+`FindResource`; `.rsrc`
  survives the Steam `.bind` DRM) or cache under `%APPDATA%`.
- Artifacts: `runs/extract/{sotesd,sotesp,sotesexe}` (offline PE dumps), `runs/cutscene-cast`
  (settled cast census), feed crops `cs_0xc35a_dx288` (Arche+mom) / `cs_0xc3f0_dx544` (man).
  Reusable probe: `OPENSUMMONERS_DUMP_BANK=<bank>` (needs `WSLENV` fwd) spawns a frames-row of
  `<bank>` (reverted from `main.c`; re-add the ~20-line `enter_game` block if needed).

## Where we are — ckpt 89

**The SKY-AMBIENT particles (`0x18704` = CHIMNEY SMOKE) are PORTED + USER-1:1, placement
made TRACE-FAITHFUL, and a full-intro side-by-side video shipped.**  Chip 4 of the
in-game-intro arc, built on the ckpt-88 particle pool + alpha path.  Durable in
`findings/in-game-intro.md` "The SKY-AMBIENT particles" + engine-quirk #88.

- **Ported (`src/particle.{c,h}`; 911 pass, +5; ledger unchanged — bare-VA slices).**  The
  second town particle system: emitter `0x112e2` (`0x54f980:150`, 1 spawn / 6th tick), config
  `0x557550:630` (bank `0x1aa` frame 8, clip `0x644b58` = 6-frame ONESHOT decoded from the
  exe, layer 6, `0x453960` scatter), step `0x46e510:683` (vel_y decel→-5000, integrate,
  **expire on the oneshot +0x74 done flag**, lifetime fade), render via the ckpt-88 alpha arm
  but through **ramp_b** (`0x8a9308`) not ramp_a.  `game_present_blit` PRESENT_ALPHA now
  decodes `param8 = (ramp_sel<<8)|idx`.  Wired in `main.c` (both `0x112e2` props → the shared
  `g_fountain_pp`).
- **USER-confirmed 1:1** ("smoke looks 1:1") + the USER independently spotted the chimney smoke
  in retail.  It's a translucent plume at the very top of the screen, visible after the camera
  pans left from the fountain; during the establishing shot most of the plume is behind the top
  letterbox bar.
- **Trace verification (USER directive) caught + fixed 2 RNG-independent placement bugs:**
  1. **Anchor** — I had HARDCODED `+1600` (USER caught it: "are you hardcoding the offset?").
     The faithful `0x557370` mode-1 anchor is render-state `+0xc/2`; the invisible `0x112e2`
     trigger has `+0xc==0` → **anchor 0** (spawn at the prop's exact world pos).  Removed the
     constant.  (Fountain: `+0xc`≈2810→+1405 ≠ its display-cel width 3400/+1700, so `+0xc`'s
     setter is still un-RE'd → fountain keeps PORT-DEBT `fountain-anchor`, calibrated +1245.)
  2. **Facing** — `runs/sky-facing` field-spec capture: every particle (sky+water) has
     render-state `+0x2c == 1` → `0x46e510` x-integrates `+= +vel_x/100` (no flip) → the sky's
     negative scatter vel_x makes it **drift LEFT** (matching retail world X `[50690..114356]`).
     The port spawned facing 0 → mirrored → drifted right.  Fixed: `particle_spawn_{water,sky}`
     set facing 1.  After: port sky world X `[51440..113369]` ≈ retail, Y matching.
- **The town `+0x13e0` band renders ONLY `0x18704` + `0x18708`** (trace: 3256 + 2668) — both
  ported, so **no particle remainder** in the town.
- **Full-intro side-by-side VIDEO (USER-requested).**  Frame-matched (anchor-aligned: port
  newgame 690/prologue 826/game_enter 1116 vs retail 748/945/1430) retail|port across
  title→newgame→prologue→town, 64 pairs (`/tmp/intro_sidebyside.mp4` + feed montage).
  **title/menu 1:1, prologue aligned, town establishing 1:1** (backdrop + fountain +
  decorations + townsfolk + chimney smoke).  ONE divergence: retail's **"Town of Tonkiness"
  area banner** (~retail flip 1600+) is MISSING in the port = `0x5a00c0` scripted-overlay debt
  (PORT-DEBT `ingame-nontile-layers`); a TIMED element (absent at the hold, consistent #82).
- **NEXT (toward whole-scene 1:1):**
  1. **The `0x5a00c0` banner / scripted overlay** — now precisely timed by the video (retail
     flip ~1600+).  The last clearly-visible missing town element.
  2. **Phase-match the particle RNG** (PORT-DEBT `fountain-rng-phase`) — exact per-frame
     particle positions need the co-resident per-tick consumers (`0x47b990` wander + other
     `0x54f980` cases) ported too; Phase 2.
  3. The **dark establishing-shot TOP GRADIENT** (open since ckpt 66/67).
- Artifacts: `runs/sky-facing/` (the facing capture), `runs/video-retail/` (the 67 retail
  video frames), `/tmp/intro_sidebyside.mp4`, `/tmp/videostitch/` (the 64 stitched pairs).

## Where we are — ckpt 88

**The FOUNTAIN SPRAY is PORTED + USER-confirmed** — Chip 3 of the in-game-intro arc.
RE'd the whole particle subsystem, ground-truthed it, decoded the clips, then ported
the fountain water `0x18708` + the ALPHA render path.  **USER-confirmed: "the particles
blending looks correct."**  All durable in `findings/in-game-intro.md` "The FOUNTAIN
SPRAY" + engine-quirk #87.

- **PORTED (`src/particle.{c,h}`, NEW; 906 pass +8, ledger 199/194 unchanged).**  The
  `+0x13e0` band as a 1024-slot `particle_pool` (alloc `0x557370`, round-robin free-slot;
  evict-oldest deferred PORT-DEBT(particle-evict)).  The fountain water `0x18708`: config
  `0x557550` (bank `0x1aa` frame 6 + the decoded clip `0x6449c0`), emitter `0x54f980` case
  `0x112e5` (`particle_fountain_emit`: 1 droplet/primary-tick, UP+OUT 3-way velocity cycle,
  6 LCG draws), step `0x46e510` case `0x18708` (`particle_pool_step`: gravity +8000/tick,
  integrate, fade sub_phase 0→8, expire).  Extended `actor_render_state` with vel_y(+0x18)/
  vel_x(+0x28)/sub_phase(+0x58)/life(+0x5c).
- **The ALPHA render (the key fix; the d3d trace nailed it).**  The first pass rendered the
  particles OPAQUE (reused the keyed `actor_render_static`) — the USER saw they lacked
  retail's transparency.  Tracing the emit (`0x4917b0`, run `runs/fountain-alpha`) showed
  retail emits the particle MODE-1 (alpha), param8 = a brightness DESCRIPTOR pointer =
  `&DAT_008a92e0[-sub_phase]`.  Decisive: `0x8a92e0 = &g_pd_boot_group_a[10]` (group_a is
  20×4 B at `0x8a92b8`), so the fade = **`g_ramp_a[10 - sub_phase]`** — and the port
  already builds `g_ramp_a`.  `particle_pool_render` now emits mode-1 nodes (param8 = the
  ramp index), `map_present` case 1 + `game_present_blit` PRESENT_ALPHA orchestrate via
  `zdd_blit_orchestrate` (mirrors `title_render` alpha_blit) → translucent droplets.
  Retires part of PORT-DEBT(present-actor-modes).
- **WIRED (`main.c`):** `g_fountain_pp`; `enter_game` finds the `0x112e5` prop, caches the
  emit center (prop world pos + `FOUNTAIN_EMIT_X_OFF` 1245 = the ground-truth spray
  center; PORT-DEBT(fountain-anchor) = the real `0x426620` body half-width);
  `game_actor_update` emits+steps each sim-tick; `game_actor_walk` renders.
- **NEXT (toward whole-scene 1:1):**
  1. **Phase-match the particle RNG** — PORT-DEBT(fountain-rng-phase).  The spray is
     visually correct but its exact particle positions differ from a given retail capture:
     the fountain shares the per-tick LCG with the co-resident consumers (`0x47b990`
     wander + the other `0x54f980` cases), which are NOT yet ported, so the stream phase
     diverges.  Under `--lockstep` + the ckpt-86 re-pin the scene RNG is deterministic
     (quirk #87), so porting ALL per-tick consumers in order → bit-exact (Phase 2).  USER:
     "if the particles can be phase matched, this is likely 1:1."
  2. **The dark establishing-shot TOP GRADIENT** the USER sees in the retail frame — a
     per-scene cinematic effect, open since ckpt 66/67 (distinct from the letterbox #74).
  3. **The `0x18704` sky-ambient particles** (emitter `0x112e2` `0x54f980:150`, frame 8,
     layer 6, clip `0x644b58` 6-frame oneshot) — the other half of the ~58-69/frame count.
  - Artifacts: `runs/fountain-alpha/` (the retail PNG + the alpha-trace);
    `runs/rng-census-repin/` (run A ground truth).

- **The architecture (5 parts, decompile-read).**  The `+0x13e0` DEVICE band is a
  **1024-slot particle pool** (`0x46cd70:103` walks it, calls `0x46e510` per slot):
  1. **alloc** `0x557370` — round-robin to the first free slot (`+0x1d0==0`, cursor
     `& 0x3ff`), else evict the oldest (lowest `+0x280`); position parent-relative by
     anchor mode (1=center/2=top/3=center-top/4=center-bottom).
  2. **config** `0x557550` (a 21 KB per-code switch) — installs bank/frame (`0x426d70`),
     clip (`0x407b80`), optional launch-velocity scatter (`0x453960`, 2 draws), body
     (`0x426620`).
  3. **per-tick** `0x46e510` (10.7 KB switch on `+0x1d4`) — integrate `x += ±vel/100`
     (signed by facing `+0x2c`), `y += vel_y/100`; age vel_y toward a clamp (gravity);
     cycle the clip; fade via the alpha LUT `&DAT_008a9308` into `actor+0xf4/+0xf8`;
     expire on lifetime or a collision-grid hit (`(x,y)/0xc80 → mapctl+0x21c04`).
  4. **render** `0x493480` default arm — `0x44d160` describe → `0x4917b0` **ALPHA-blit**
     (brightness `actor+0xf4`).  (Its `0x186ca` arm is a separate cel-string renderer,
     not a particle.)
  5. **emitters** = CHARACTER props the port ALREADY spawns (ckpt 79): the fountain
     `0x112e5` (`0x54f980:218`) spawns one **`0x18708`** water droplet each primary
     sim-tick; `0x112e2` (`:150`) spawns a **`0x18704`** sky particle every 6th tick.
- **Live ground truth (run A `runs/rng-census-repin`, `0x493480` 5924 render calls; both
  bank `0x1aa`=res `0x408`).**
  - `0x18708` — fountain WATER: clip `0x6449c0` (base0/count2/dur2/loop {0,1}), frame_base
    6, **layer 11**, a tight ~158px column at the fountain (x 1697-1855, center ~1772,
    denser at top y≈357 → falling to y≈461).
  - `0x18704` — SKY ambient: clip `0x644b58` (base0/count6/dur20/ONESHOT {0,1,2,3,4,5}),
    frame_base 8, **layer 6**, wide upper area (x 507-1143, y ≈ -17..184).  A separate
    system from the fountain.
  - ~58-69 particles alive/frame.  Clips decoded from the exe (decoder validated vs
    IDLE_CLIP, exact).
- **Parity bar (corrects a first mis-read; refines quirk #77).**  Filtered to the actual
  LCG (`va==0x5bf505`; an earlier histogram wrongly included the blit hooks), the hold
  stream is REGULAR per-sim-tick — 238 at the spawn tick (#86) then a period-6 cycle
  `[6,14,6,14,14,14]` consumed only by per-sim-tick updaters
  (`0x54f980`/`0x47b990`/`0x453960`, the last ≈1 particle spawn/tick).  **The render
  draws NO LCG.**  In `--lockstep` (1 present/tick = the port's cadence) there is no
  per-present variance, so under the re-pin the spray is **bit-exact portable, CONTINGENT
  on reproducing the per-tick consumption ORDER** — entangled with the co-resident
  consumers (`0x47b990` wander + other `0x54f980` cases), i.e. the broader Phase 2.  A
  second seed-pinned run to prove run-to-run determinism is blocked on the title→town
  input automation; settle it directly at verification (render_diff vs retail).
- **RESOLVED the ckpt-88-RE open detail** (`0x18708`'s 158px x-span vs the ±4px spawn
  jitter): the velocity is set in the `0x54f980:260-283` branch where `param_3` is
  REASSIGNED to the just-spawned droplet (not a sub-entry) — the up+out launch + gravity
  makes the parabolic arc, ported faithfully.

## Where we are — ckpt 87

**The townsfolk IDLE ANIMATION PHASE is PORTED** — built directly on the ckpt-86 RNG
anchor.  The 11 standing villagers now run the idle breathing clip from a per-actor
RNG start frame (matching retail's staggered phases) instead of frozen on frame 0.
The first user-visible payoff of the spawn-RNG arc.

- **The model (engine-quirk #86, decompile- + census-verified).**  For each of the
  15 map EFFECT objects in dispatch (= map-layer) order, `0x41f200` consumes a fixed
  per-object RNG burst before its `0x426ec0` idle-phase pair:
  `0x426fd0`(1) + prologue(7) = 8, PLUS the per-CODE type-switch draw — 5 for the 4
  `0xe29a` wanderers (`0x427670` case 2, decompile-confirmed :2181), 1 for `0xe2a5`
  (`0x431cb0`, :2272), 0 for the plain villagers — then `0x426ec0`(2):
  `frame = (rand * clip.frame_count) >> 15`, `timer = (rand * clip.frame_dur) >> 15`.
  The shared idle clip is `0x6290e0` (decoded from the exe: base 0, **20 frames, dur
  14**, looping, sprite delta {0,1,2,1,0,1,2,1,0,1,2,3,0,1,2,1,0,3,2,3}, zero offsets).
  Crucially, **all 11 rendered townsfolk are in the first 15 (map) effects**; the 4
  script effects (`0xc35a`×2/`0xc3dc`/`0xc3f0`) + the conditional `0x41f200:2849` draw
  spawn AFTER them, so they do not affect the townsfolk phases.
- **Ported.**  `actor_spawn_effect_from_map` (`actor_spawn.c`) now walks EVERY map
  EFFECT object in order and replays its draws (consume-to-advance via `rng_rand`; the
  jitter/particle values feed unmodelled fields, only the `0x426ec0` pair is USED, and
  only for the rendered townsfolk — the `0xe29a` wanderers + unknown codes still consume
  their draws but are not spawned).  Embeds `IDLE_CLIP` + `effect_prefix_draws(code)`.
  Sets `rs->clip = &IDLE_CLIP`, `rs->frame`/`rs->timer` from the phase draws.  `main.c`
  `game_actor_update` also advances `g_effects` per sim-tick (RNG-free
  `anim_clip_advance`) so they breathe in lockstep with the protagonist's horses.
- **Verified.**  Host test `actor_spawn_effect` locks the replay to an inline reference
  LCG (exact frame/timer per slot) — **898 pass**.  The draw model is census-verified
  (counts 134/38/20/19, the per-object shape RLE in `runs/rng-census-repin`) +
  decompile-verified (the `0xe29a`/`0xe2a5` cases).  Offline from the pinned `0x4f5347`
  the 11 start frames are {1,17,17,17,3,14,4,16,18,12,10}, timers
  {3,13,10,4,7,5,10,1,12,0,1}.  Ledger 199/194 unchanged (bare-VA slice of `0x41f200`).
- **VALIDATION PENDING — NOT yet differ_px==0.**  The chain (Chip-1 seed + census draws
  + decompile shapes + decoded clip + host test) is complete, but the live bit-exact
  cross-check has not run.  **Do this first next:** capture retail's `+0x72`/`+0x70`
  per townsperson under the re-pin and compare to the offline values above.  Cleanest
  routes: (a) add a `0x426ec0` **onLeave** field read (arg0+0x72/+0x70) — the field-spec
  currently reads onEnter only (`ctReadField`), so this needs a small onLeave path in the
  agent; or (b) render_diff the townsfolk cels (res,frame) at a matched sim-tick (port
  capture + retail re-pin capture).  If a townsperson's frame mismatches, it pinpoints
  the first wrong object's draw count.
- **NEXT (Chip 3) — the FOUNTAIN SPRAY** (band `+0x13e0` / `0x493480`, res `0x408`): the
  spawn particle init (`0x41f200`'s `0x427b70` params `:326-334` + the `0x427670`(20)
  helper) + the per-tick `0x47b990`/`0x453960` update + the 4 `0xe29a` wander.  All RNG,
  riding the same re-pinned stream.

## Where we are — ckpt 86

**The town SPAWN RNG ANCHOR is LANDED + LIVE-VERIFIED** — the keystone that makes the
two ckpt-85 RNG residuals (townsfolk idle PHASE + the fountain SPRAY) portable.  RE +
tooling milestone; no visual change yet (that lands when the spawn consumers are
ported, Chip 2).

- **The problem (quirk #77, re-confirmed).**  The title→town RNG is non-deterministic
  run-to-run even under the boot title seed-pin: a per-present consumer desyncs the
  shared LCG between the title sparkle pin and the town, so `game_enter`'s seed was
  `0x46fe3f46` this run vs `0x83600390` last.  The town SPAWN draws (idle frame,
  particle jitter) therefore started from an unpredictable phase.
- **The spawn structure (retail ground truth, engine-quirk #86; the seed-pinned
  `0x5bf505` census `runs/rng-census-repin`, `--split-frame 1419`).**  The town-LOAD
  frame draws a fixed **238-draw burst over 19 EFFECT objects** (`0x58d460` →
  `0x41f200`, map-layer order).  Per object: `0x426fd0`(1, the `+0xf4` init) +
  `0x41f200`(7 = 2 position-jitter `:294`/`:301` + 5 particle-param `:326-334`) +
  **optionally `0x427670`(5)** (per-case particle init; 4 of the 19 are this "shape-2",
  15 draws; the rest "shape-1", 10 draws) + `0x426ec0`(2 = idle frame `+0x72` / timer
  `+0x70`).  Two objects carry a type-switch one-off (`0x431cb0`/`0x427360`); one fires
  the conditional `0x41f200:2849`.  Stable counts: 134/38/20/19.  **The port renders 11
  standing townsfolk but ALL 19 consume RNG**, so the Chip-2 replay must process all 19
  in map order (rendering only the 11) to keep the idle phases aligned.
- **The re-pin point — the FIRST `0x41f200`, NOT `game_enter`.**  A pre-spawn one-off
  `0x4c5e00`(1 draw) fires between the `game_enter` entry (`0x59f2c0`) and the first
  `0x41f200`, so a `game_enter` pin would leave the spawn one draw out of phase vs a
  port that replays only the `0x41f200` burst.  Verified: at the first `0x41f200` the
  natural seed was `0x71cc78f1` (≠ `game_enter`'s `0x46fe3f46` → the intervening draw is
  real).
- **Implementation.**
  - **Agent (`opensummoners-agent.js`):** `installRngAnchor()` hooks `0x41f200`;
    `g_rng_anchor_armed`/`g_rng_anchored` armed in the `game_enter` scene-anchor
    handler, fired (write `DAT_008a4f94 <- 0x4f5347`) at the first `0x41f200` onEnter,
    per-map latch; emits the `rng_anchor` event.  `frida_capture.py` logs it.
  - **Port (`main.c`):** `game_rng_seed()` helper (default `OSS_RNG_DEFAULT_SEED`, env
    `OPENSUMMONERS_RNG_SEED`; shared by boot + `enter_game`) + `rng_srand(game_rng_seed())`
    re-seed at the top of `enter_game` — the faithful mirror, since all pre-effect-spawn
    `enter_game` code (load_town_scene, camera, CHARACTER/STRUCTURE spawns) is RNG-free,
    exactly as retail's `0x431e30`/`0x438a60` dispatches are.
- **Verified live** (`runs/rng-census-repin`, `--seed-pin --lockstep --no-turbo`):
  `town SPAWN RNG re-pinned @ frame 1419 (game_spawn): DAT_008a4f94 0x71cc78f1 ->
  0x004f5347`.  Spawn draw counts byte-identical pre/post (134/38/20/19) ⇒ the re-pin
  resets seed VALUES only, control flow untouched.  The spawn is now deterministic from
  `0x4f5347` on both sides.  **898 pass, ledger 199/194 unchanged** (harness + a port
  seam, no function ported).  quirk #86; `findings/in-game-intro.md` "The town SPAWN RNG
  anchor".
- **NEXT (Chip 2 — the idle PHASE, the first user-visible payoff):**
  1. Port `0x41f200`'s per-object RNG consumption in map order for all 19 EFFECT
     objects (consume-to-advance: replay the right draw count/order per object shape;
     only the values feeding the idle phase + later the particles are USED).  The
     shape (10 vs 15 draws) is keyed on the object CODE — determine which codes hit the
     `0x427670` cases (likely the 4 wandering `0xe29a`) + the `0x431cb0`/`0x427360`
     one-offs.
  2. Give the 11 townsfolk the idle clip `0x6290e0` (reconstruct it from the exe like
     the wagon clip `0x671c48`, ckpt 80; ~20 frames dur 14 looping per ckpt-85) and set
     render-state `+0x72`/`+0x70` from the aligned `0x426ec0` draws (frame =
     `(rand*frame_count)>>15`, timer = `(rand*frame_dur)>>15`).
  3. Verify vs retail: capture render-state `+0x72` per townsperson (a `0x493ba0`
     field-spec `rs_frame` read) under the re-pin and compare to the port.
  - Tooling: `rng_consumer_census.py --order`, `flow_diff.py`, the `0x5bf505`
    `rand_draw` field + `rngcalls`.  Fresh ground-truth trace:
    `runs/rng-census-repin/call_trace.jsonl` (re-pinned, game_enter@1419).
- **Then Chip 3 — the fountain SPRAY** (band `+0x13e0`/`0x493480`, res `0x408`): the
  `0x41f200` particle params (`:326-334` → `0x427b70`) + `0x427670`(20) at spawn, then
  the per-tick `0x47b990`/`0x453960` update + the 4 `0xe29a` wander.

## Where we are — ckpt 85

**Townsfolk FACING is PORTED + USER-confirmed 1:1 — and it's a deterministic MAP
field, NOT RNG (correcting the ckpt-84 guess).**  Phase-2 "matching half", first
chip; built on the ckpt-84 census.

- **Facing = the map sub-record `puVar1[4]` (RNG-free).**  Dispatcher
  `FUN_0058d460:96` computes `cVar12 = (puVar1[4]!=0)?3:1` (1 normal / 3 mirrored)
  and forwards it as **param_8** to the EFFECT activator `0x41f200` (`:151`) + the
  CHARACTER activator `0x431e30` (`:227`); `0x41f200:861` stores it at render-state
  `+0x2c`.  `FUN_0044d160` mirrors only on `facing==3`: cel `frame += flip`,
  `off_x = mirror_x - off_x`, where `flip = *(s16)(DAT_008a8440[bank])`.  **`0x8a8440`
  is a POINTER array** (confirmed live — each cell derefs to a heap sprite-group
  descriptor) whose first short = the group's **frames-per-direction** (4 or 16 for
  the town banks).  So the mirrored cel = `frame_base + frames_per_dir`.
- **Ground truth** (live `0x493ba0` census + a new `rs_facing` field + a one-shot read
  of `DAT_008a8440` via `runs/read_fliptable.py`): of the 11 map townsfolk **7 are
  facing 3** (`c3be/c3dd/c3e6/c422/c42c/c441/c468`), 4 normal (`c3f2/c404/c440/e2a5`);
  flip per bank = {0xd4:16, 0xe1:4, 0xe5:4, 0x93:16, 0x99:16, 0xa9:16, 0xd0:4, …}.
- **Ported (898 pass, builds clean):** `TOWN_EFFECT_DEFS` gains `facing`+`flip`
  columns; `actor_spawn_effect_def_for_code` returns them; the spawn sets
  `rs->facing`; **`actor_spawn_effect_fill_flip_table`** fills a bank-indexed
  stand-in for the global `DAT_008a8440`, wired in `main.c` and passed to every
  `game_actor_walk` `actor_render_static` call.  **USER-confirmed: "npc orientation
  matches retail yes."**  PORT-DEBT `effect-sprite-table` extended (facing+flip
  captured; proper source = map `puVar1[4]` via the unported `0x587e00` + the
  `DAT_008a8440` frames/dir).  quirk #85; `findings/in-game-intro.md` "Townsfolk
  facing is a MAP field".
- **The remaining TWO residuals are RNG → need the game_enter RNG ANCHOR:**
  1. **Idle PHASE** — `FUN_00426ec0` sets `rs+0x72 = (rand()*clip.frame_count@+0x42)
     >>15` (every townsperson runs the idle clip `0x6290e0` at a random start frame)
     + a 2nd rand for the timer.  The port still spawns them clip=NULL (frozen).
  2. **The FOUNTAIN SPRAY** (band `+0x13e0` / `0x493480`, res `0x408`) — `0x41f200`'s
     8 rand draws are position-jitter (`:294`/`:301` → `0x426e00` `+0x58`/`+0x60`) +
     a particle sub-spawn (`:326-334` → `0x427b70`); helper `0x427670` (20 draws) +
     per-tick `0x47b990`/`0x453960` drive the spray.
- **NEXT (the RNG anchor — the keystone for both):** re-pin `DAT_008a4f94` to a fixed
  value at the `game_enter` anchor on BOTH sides (retail: a new `frida_capture`
  re-pin at the anchor; port: re-seed in `enter_game`), then port the spawn RNG
  consumers IN ORDER (`0x41f200` jitter → `0x426ec0` phase → `0x427670`/`0x427b70`
  particles), `flow_diff` to localize the first divergent draw, → the idle phases +
  fountain land 1:1.  Then the per-tick consumers + `0xe29a` wander.  Tooling:
  `rng_consumer_census.py`, `flow_diff.py`, the `rng_state`/`rngcalls` fields,
  `rng_tick_diff.py`.  Artifacts (ephemeral): `runs/facing-census/`,
  `runs/read_fliptable.py`.

## Where we are — ckpt 84

**The EFFECT townsfolk are PORTED (positions USER-confirmed 1:1) — and the scene's
residual is now PINNED to the RNG pillar, so Phase 2 (the RNG-consumer census)
begins.**  Builds on the ckpt-83 census; the wagon/STRUCTURE precedent
(position-first, animate next).

- **Render REUSES `actor_render_static`.**  For a plain townsperson `FUN_00493ba0`'s
  static arm (`LAB_004943d7` → `FUN_0044d160` describe → emit loop) emits exactly ONE
  mode-0 keyed cel — verified vs the hold blit trace (`0x5b9b70` carried `res`+`frame`;
  18 keyed blits, one per townsperson; **no `0x4917b0` shadow, no `DAT_008a9358`
  color-remap** — the node stayed keyed mode-0).  The spell-text/HP-bar/effect-list
  arms gate on render-state `+0x66` / view `+0x562a` (0 here) → skipped.  So the EFFECT
  band reuses `actor_render_static`, like STRUCTURE.
- **Placement FULLY MAP-DRIVEN** (census + map, no 27 KB switch read): `world = (map
  (x,y) − dst) × 100`, `dst` = the per-code render anchor (rs `+0x40/+0x44`).  Verified
  cel-for-cel: census `rs_x` = `(map_x − dstx) × 100` (e.g. `0xc3e6` (208,384) dst
  (−30,−32) → (23800,41600)).  The +30 world offset cancels the −30 render dst →
  `screen = map − cam`.  11 map townsfolk = 10 `0xc3xx` + `0xe2a5`.
- **Ported (898 pass, +1):** `actor_spawn_effect_from_map` + `actor_spawn_effect_def_
  for_code` (`actor_spawn.c`; PORT-DEBT `effect-sprite-table` — captured `{code → bank,
  dst, layer}` stand-in for `0x41f200`); `main.c` `g_effects` spawned in `enter_game`,
  walked by `game_actor_walk` via `actor_render_static` at layer 13.  Live: 11 spawn +
  11 emit (bank `0xf9` registered).  Ledger 199/194 unchanged (bare-VA slices).
- **USER-confirmed: "the NPCs are rendering at the correct positions."**  render_diff
  (port 1200 ↔ retail 1500): on-screen townsfolk match on res + position (zero
  `[rect]`/`[state]`).
- **THE RNG RESIDUAL (USER directive — pivot to Phase 2).**  The scene is NOT yet 1:1
  every frame because of THREE RNG-driven elements:
  1. **Townsfolk FACING (mirror)** — some render flipped (USER: "the orientation of
     some of them is flipped").  `FUN_0044d160`'s `rs->facing == 3` arm reflects `off_x`
     + picks the mirror cel (`frame_off = DAT_008a8440[bank]`); the port spawns facing 0
     + passes `flip_table NULL` → NO mirror.  Facing set per-actor (likely RNG at spawn).
  2. **Townsfolk idle PHASE** — frozen frame 0.  Clip `0x6290e0` (base 0, 20 frames,
     dur 14, looping, delta {0,1,2,1,0,…}) + the stepper ARE ported; the per-actor START
     phase is staggered run-deterministically (`0xc3e6` f0 / `0xc404` f18 / …; NOT a
     map-record field → likely RNG at spawn).
  3. **The FOUNTAIN PARTICLE SPRAY** — the entire `+0x13e0` band (`0x493480`, res `0x408`
     bank `0x1aa`) is MISSING (USER crop: a purple/blue/white sparkle spray erupting from
     the fountain + green leafy particles right).  Clearly RNG (particle pos/vel).
- **RNG-CONSUMER CENSUS — DONE + integrated into the flow trace** (USER directive
  "integrate, not a bespoke probe").  Added `0x5bf505` (the LCG) as a
  `retail_fields.json` entry (the auto `ret_va` names the consumer site) +
  `tools/rng_consumer_census.py` (ret_va+0x400000 → function via `functions.csv`,
  split spawn/hold).  Captured 1142 town-scene draws (game_enter@1434); enumerated
  EVERY consumer, cross-checked vs the decompile (`0x41f200` has exactly 8 rand calls
  = the 8 sites):
  - **SPAWN (room-load, one-shot):** `FUN_0041f200` (the EFFECT activator, 134 draws /
    8 sites — the townsfolk facing + idle phase + a per-object 7-draw cluster
    `0x41f200:294-336` feeding `0x426e00` pos/scale + `0x427b70` particle sub-spawn) +
    helpers `0x426ec0`/`0x427670`/`0x426fd0` (77).
  - **HOLD (per-tick):** `FUN_0054f980` (behaviour/wander, 425), `FUN_0047b990` (the
    `+0x1160` EFFECT-band update — fountain particles / wander, 320), `FUN_00453960`
    (154).
- **NEXT (the MATCHING half — Phase 2 cont.):**
  1. **RE each consumer's draw semantics** — `0x41f200`'s 8 sites (which sets facing
     `+0x2c` — note it reads `param_8`, so trace the facing's randomness UPSTREAM — vs
     idle phase `+0x72` vs particle init); then `0x47b990`/`0x453960` (the fountain
     spray) + `0x54f980` (wander).
  2. **An RNG ANCHOR at game_enter, both sides** (the ckpt-73 fix): the port can't
     replay the whole boot RNG chain, so snapshot/restore `DAT_008a4f94` to a known
     value at the town-scene entry on BOTH sides (retail: a new agent re-pin at the
     `game_enter` anchor; port: re-seed in `enter_game`), so the town RNG starts
     aligned.
  3. **Annotate each producer** with a `rngcalls` field + a port `CALL_TRACE_BEGIN`
     mirror, then `flow_diff` localizes the first divergent draw → port the consumers
     in order → the facing/phase/particles/wander go 1:1.
  Best started fresh — orient: CLAUDE.md → FRONT → here → `findings/in-game-intro.md`
  "The scene-wide RNG-consumer census".  Tooling: `tools/rng_consumer_census.py`,
  `tools/flow_diff.py`, `tools/rng_tick_diff.py`, the `0x5bf505`/`rng`/`rngcalls` field
  sources (`retail_fields.json`).
- Artifacts (local, ephemeral): `runs/rng-census/` & `/tmp/rng_census/call_trace.jsonl`
  (the RNG census), `/tmp/cast_census/call_trace.jsonl` (retail hold cast census),
  `/tmp/eff_port_trace.jsonl` (port blit trace @1200), `/tmp/decode_clips.py` (clip
  decoder), `/tmp/eff_objs.py` (the world=(map−dst)×100 cross-ref).

## Where we are — ckpt 83

**The establishing-hold CAST is PINNED to its producers — Phase 1's complete
producer map.**  RE + live-census milestone (no port yet); resolves the ckpt-82
"pin the cast source next" open item and decomposes the scene for porting.

- **Method.**  Field-spec **band census**: added the 5 non-main band render
  entries (`0x4937c0`/`0x493480`/`0x492fc0`/`0x493230`/`0x493ba0`) + the two emit
  primitives (`0x492670`/`0x4917b0`, with `renderid` on the emitted cel) to
  `tools/flow/retail_fields.json`, captured at the hold (flips 1450/1500/1600,
  `--seed-pin --lockstep --no-turbo`).  The driver `0x48c150` (free-roam branch)
  runs 8 emit passes over the `DAT_008a9b50` bands → one present `0x48eac0`; the
  0x3c draw-node carries cel+pos+mode only (no producer back-ref), so the
  cel↔producer tie is read at EMIT.  The emit-primitive `cel_res` hook is
  authoritative (it caught a VA-arithmetic footgun — see CAVEAT).
- **The 18 visible keyed cels = FOUR map-object bands (all DATA-1022):**
  - **`+0x2560` STRUCTURE → `0x493230`** (single-cel renderer): the **TREE**
    `0xec55` bank `0x15f`→res `0x481` (×2); **bg decorations** `0xec6a` bank
    `0x16c`→`0x403` (×29, layer 8); **fg hedges** `0xec60` bank `0x164`→`0x426`
    (×8; the 5 on-screen are **layer 15** = the bottom row).
  - **`+0x1160` EFFECT → `0x493ba0`** (multi-part char render, built on the ported
    `0x44d160`): the **townsfolk** — 10 distinct `0xc3xx` (1 each) + `0xe29a`×4 +
    `0xe2a5`, banks `0x8b`–`0x146`→res `0x459`/`0x462`/`0x46a`/`0x46b`/`0x472`/
    `0x47b`/`0x426`/`0x3fa`.  layer 12/13.
  - **`+0x11e0` CHARACTER → `0x491ae0`**: collision volumes (bank 0) + props
    (`0x16c`→`0x403`) + the script wagon `0x1872d`.  **Already ported.**
  - **`+0x13e0` → `0x493480`**: 41 animated bank-`0x1aa`→res `0x408` particles
    (layer 6 sky + a square cluster) — blit via alpha/clipped, **NOT in the keyed
    set**; deferred.
- **Map-driven + deterministic (the key result for porting).**  STRUCTURE
  render-state = DATA-1022 record EXACTLY: world pos = map `(x,y)`×100,
  **`frame_base` = map `variant`@+0x18** (verified cel-for-cel: tree {0,1}, hedge
  {0,1,4,5}, deco {16,18,20,21,24,26,28,32,33,35} identical live-vs-map).  The
  code→bank map is the activator's per-type def table (lazy `+0x48` fill, #80):
  `0xec55`→`0x15f`, `0xec60`→`0x164`, `0xec6a`→`0x16c`.  EFFECT townsfolk map 1:1
  by code/count (`map_data --objects`: 13 EFFECT codes incl. `0xe29a`×4) but carry
  a deterministic spawn offset (≈+3000 x) from the `0x41f200` activator.  `0xc35a`
  (×2, also drawn by the party renderer `0x4997b0`), `0xc3dc`, `0xc3f0` are NOT in
  the map → script/party-spawned (like the wagon).
- **Hold = mostly STATIC.**  Across flips 1450/1500/1600 (cam 128000, pre-pan) the
  16 standing townsfolk + 39 structure objects hold a FIXED world pos (only the
  anim frame steps, deterministic per #76); only `0xe29a`×4 translate (RNG wander,
  Phase 2).  Refines #82: the +0x1160 EFFECT band updates during the hold.
- **Corrects the docs' model:** the visible cast is NOT the +0x11e0 band (that's
  collision volumes + props); it splits across +0x1160 (townsfolk) / +0x2560
  (tree/scenery) / +0x11e0 (props) / +0x13e0 (particles).  The "foreground tree"
  is STRUCTURE `0xec55` (res `0x481`) — NOT a banner / `0x5a00c0` overlay / tile
  (all refuted).
- **CAVEAT (footgun):** hand-computing the band VAs for the census analysis was
  wrong (off by 0x200) → bands falsely read "0 active".  Compute VAs in code;
  trust the emit-`renderid` hook over a band-entry census.  quirk #84.
- **State: 896 pass unchanged** (no C touched; docs + `retail_fields.json` only).
  Ledger unchanged.  quirk #84; `findings/in-game-intro.md` "The establishing-hold
  cast is FOUR map-object bands".
- **STRUCTURE band DONE (ckpt 83b) — PORTED + USER-1:1.**  RE'd `0x438a60`
  (per-code bank def table: 0xec55→0x15f, 0xec60→0x164, 0xec6a→0x16c, …) + the
  `0x58d460` dispatch (layer = record +0x30 ? 15 : 8; frame_base = variant +0x18;
  pos = (x,y)×100 — all verified cel-for-cel vs the census).  PORTED
  `actor_spawn_struct_from_map` (60000-range, fully map-driven) +
  `actor_spawn_struct_bank_for_code`; the RENDER reuses `actor_render_static` (the
  `0x493230` static single-cel blit is bit-identical to the default actor arm).
  Wired via `game_actor_walk` (walks g_structs at layers 8/15) + spawned in
  `enter_game`.  **Live: 39 objects spawned + 39 nodes emitted (tree bank 0x15f
  registered); render_diff/position-verified bit-exact** (tree `(0x481,f0)`@(496,64)
  320×320, the 5 hedges, the 4 `0x403` props/deco — all identical port↔retail, zero
  `[rect]`/`[decode]`/`[state]`).  **USER-confirmed on the feed: "the decorations
  are there and positioned 1:1".**  897 pass (+1 `actor_spawn_struct`); ledger
  199/194 unchanged (bare-VA slices); parity-ledger #9.
- **NEXT (Phase 1 cont.) — the EFFECT townsfolk (the people in the square):**
  1. **The multi-part char renderer `FUN_00493ba0`** — built on the ALREADY-PORTED
     `0x44d160` (actor_render_describe) + `0x492670` (draw_pool_emit_actor) + the
     shadow/color-split layers via `0x4917b0`.  Port its core static arm
     (`LAB_004943d7` → describe → the emit loop); the 16 standing townsfolk are
     static (clip-animated frame only, deterministic per #76).
  2. **The EFFECT spawn `FUN_0041f200`** (the 50000-range activator, 0x58d460:151)
     — map pos + the deterministic ≈+3000 x offset (RE it; the static townsfolk
     land at a matched sim-tick).  code→bank def table (banks 0x8b–0x146).
  3. The 4 wandering `0xe29a` need Phase 2 (RNG, deferred).  Then the `0x4962a0`
     off-screen invisibles + the `0x13e0` bank-0x1aa particles (alpha).
- Artifacts (local `/tmp`, ephemeral): `/tmp/cast_census/`, `/tmp/tree_emit/`,
  `/tmp/port_hold_trace.jsonl`, `/tmp/retail_hold_1500.png`; analysis
  `/tmp/census_fixed.py`, `/tmp/parse_variant.py`.  Regen via the field-spec
  capture above.

## Where we are — ckpt 81

**The caravan's HORSES now TROT — the per-tick actor animation is wired and
BIT-VERIFIED live.**  Builds directly on ckpt 80 (the frozen wagon); the trot is
the first thing the per-sim-tick actor UPDATE pass drives in the port.

- **The RE that made it safe (engine-quirk #82).**  `FUN_0054f980`'s case-`0x1872d`
  (`:911-970`) splits cleanly: **`:911-928` is the frame-stepper, run
  UNCONDITIONALLY** (gated only on the clip `+0x6c != 0`; byte-identical to
  `anim_clip_advance`, reads no RNG/clock), and **`:929-970` is the behaviour**,
  which `break`s out unless this is the primary entry AND the global scene-lock
  `*(DAT_008a9b50+0x27a8)==0`, then draws the LCG (`FUN_005bf505`) for idle waits /
  wander — the RNG layer deferred by ckpt 73 / quirk #77.  So the horse-trot is a
  pure deterministic function of sim-ticks and portable in isolation; the wander
  stays deferred.  Driver `FUN_0046cd70:123-169` walks the 0x80-slot `+0x11e0` band
  once per sim-tick calling `0x54f980` per active actor.
- **PORTED (pure + host-tested; +3 tests, 896 pass).**
  - `actor_render_state` gains the anim sub-block `timer` (+0x70) / `done` (+0x74)
    — it already had `clip` (+0x6c) / `frame` (+0x72); the slice the stepper writes
    IS an `anim_state`.
  - **`actor_anim_advance`** (`actor_render.c`) — the per-actor stepper; a thin
    adapter bridging the render-state's anim block to the single ported stepper
    `anim_clip_advance` (one source of truth, host-tested bit-exact ckpt 72).
  - **`actor_pool_update`** (`actor_spawn.c`) — the `0x46cd70` main-band walk:
    advance every active render-state with a clip; the 32 static actors (clip NULL)
    no-op, only the wagon trots.  Returns the count advanced.
  - `main.c game_actor_update` runs it on the SAME sim-tick gate as the camera
    easer (`(g_game_camera_hold & 1)==0`), BEFORE `camera_follow_step` — mirroring
    retail's `0x439690` per-tick body order (`0x46cd70`@:1108 then `0x43d1d0`@:1123).
    `CALL_TRACE_BEGIN(0x46cd70)` port mirror (emits `advanced`).  Reset is automatic
    (`enter_game` re-spawns the pool frame/timer 0 + zeroes the hold counter).
- **LIVE-VERIFIED at the byte level** (port blit trace, settled cam 12800, flips
  2100-2244 = one 144-Flip clip cycle): the wagon (bank `0x175`) is **3 keyed cels
  res `0x3ec`** at screen x 160/288/416 (the −256/−128/0 composite); the body cel
  (x416) steps **5→2→3→4→5** (one body-frame per 36 Flips = 18 sim-ticks) while the
  two fixed wagon cels hold frames 0/1; the `0x46cd70` mirror reports `advanced:1`
  each tick.  **CORRECTION:** the wagon's render_id is **res `0x3ec`** (asset_register
  idx 215), NOT `0x058f` as ckpt-80 noted — fixed in FRONT/quirk #81.  **USER-CONFIRMED
  on the feed:** "the horses' ears animate slightly, looks correct.  The wagon doesn't
  move — the horses are just idling, which is how it's supposed to be."  So `WAGON_CLIP`
  is a SUBTLE IDLE loop, the wagon is PARKED at the settled hold (no locomotion), and the
  "trot" wording is shorthand for that idle cycle (quirk #82).
- **State: 896 pass / 0 fail / 6 skip** (+3).  Ledger **199/194 unchanged** (the
  stepper/walk are bare-VA slices of `0x46cd70`/`0x54f980`; `anim_clip_advance` was
  already counted).  quirk #82; PORT-DEBT `actor-protagonist-clip` narrowed (the
  trot half is done; the RNG behaviour + the cutscene roll-in remain).  Writeup:
  `findings/in-game-intro.md` "The horses TROT".
- **NEXT (corrected ckpt 82 — the "(b) siblings" lead was a DEAD END):** the
  code-adjacent actors `0x1872e`/`0x1872f`/`0x18730` are **OUT-OF-SCENE** (statically
  proven): `0x1872e`←`FUN_00539e80` `case 0x64280` = room 410240 (area 410);
  `0x1872f`←`FUN_005034b0` `case 0x382de` = room 230110 (area 230); `0x18730` = child
  of non-town CHARACTER `0x11350` (not in DATA-1022's 32 town char codes).  All four
  codes (100141-100144) are outside the 70000 map-object range, and the town script
  `FUN_004d7d80` (area-210 rooms `0x334be`…) spawns ONLY the wagon `0x1872d`.  So
  code-adjacency ≠ same scene (engine-quirk #83); the siblings are later-area beats.
  `findings/in-game-intro.md` "The caravan 'siblings' … are OUT-OF-SCENE".

- **GROUND TRUTH (ckpt 82) — the hold residual is the CHARACTER CAST + foreground
  TREE, NOT a "banner".**  Re-captured the retail blit trace + a PNG at the
  scene-LOCKED establishing hold (`game_enter@1434`, flip 1500, cam 128000;
  `--seed-pin --lockstep --no-turbo`, field-spec).  The 108 `blt_keyed` (`0x5b9b70`)
  split into TWO producers:
  - **54 VISIBLE via present `FUN_0048eac0` (`ret_va 0x48ecc2`, 18/frame)** = res
    `0x481` **320×320** @ (496,64) the foreground **TREE** + ~5-7 multi-part townsfolk
    **CHARACTERS** (banks `0x426`×5 / `0x459` / `0x462` / `0x46a` / `0x46b` / `0x472`
    / `0x47b`) + props `0x403` + tiny details `0x3fa`.  The PNG confirms a tree (right)
    + a knot of townsfolk in the square + a flowerbed.
  - **54 INVISIBLE via `FUN_004962a0` (`ret_va 0x49632a`/`5c`/`8b`)** parked at
    **dst y=572 (off the 480 screen)**, NO render-id — a scratch/HUD parked during the
    cutscene; draws nothing visible.  (Identify `0x4962a0` before porting.)
  - **No "Town of Tonkiness" banner blit at the hold** — zero `0x5a00c0`-range
    `ret_va`s → the docs' banner attribution AND the `0x5a00c0`-overlay producer guess
    are BOTH refuted (same as the letterbox turned out to be `0x48c150`).  Any
    area-title card is GDI text / a different time (TBD).
  - The cast is **NOT** the main map-object band (bank `0x16c`/`0x175`, world-x
    88200-176000 = off-screen-LEFT at cam 128000; the 6 drawing main-band actors don't
    present here).  Source = the 8 PARTY actors (`0x59f2c0`→`0x560e60`, `ret_va
    0x59f578`) and/or a scene-actor band — PIN IT NEXT.
  `findings/in-game-intro.md` "The hold residual is the CHARACTER CAST + foreground
  tree".  Artifacts (local `/tmp`): `/tmp/blit_banner_retail/`, `/tmp/spawn_disc/`.

- **USER DIRECTIVE (ckpt 82): the intro scene must render 1:1 on EVERY frame before
  moving to the next scene; THEN pinpoint + port every RNG consumer in the scene 1:1.**
  Per quirk #82 the hold is scene-locked ⇒ deterministic, so this decomposes cleanly:
  - **PHASE 1 (this residual) — the CHARACTER / multi-part static render.**  Pin each
    cast cel to its actor (annotate the emit `FUN_00492670` / the band-walk feeding
    `0x48eac0` with the actor code `+0x1d4`, recapture → cel↔actor map), RE the cast
    spawn (party `0x59f2c0` and/or the scene-actor band), port the multi-part static
    render (generalise the `0x491ae0` `0x1872d` arm past the wagon; the lazy `+0x48`
    sprite-table fill, PORT-DEBT `actor-sprite-table`) → the LOCKED establishing frames
    go differ_px==0.  Also byte-confirm the wagon (`render_diff` keyed `(res 0x3ec,
    frame)`).  The foreground TREE (res `0x481`, a static prop) is the simplest single
    cel to land first.
  - **PHASE 2 — every in-scene RNG consumer.**  Post-unlock the scene-lock clears and
    `0x54f980:929+` draws the LCG for idle/wander; RE + port every consumer and match
    consumption order (rng + rngcalls both sides, the flow trace's unified signal) so
    the post-cutscene frames stay 1:1.  (This RETIRES the ckpt-73 "defer all RNG"
    deferral — it is now scheduled, not indefinite.)
  - **Big foundational arc — best started with a fresh `/clear`** (the character render
    system underlies every scene).  Orient: this file → FRONT → here → the ground-truth
    finding above.

## Where we are — ckpt 80

**The town intro `0x1872d` is PORTED + SPAWN-RE'd + WIRED + USER-CONFIRMED — and
it's the arrival HORSE-DRAWN CARAVAN, not "the protagonist" (corrects #79/#80).**

- **Render arm (commit `af31c69`):** `actor_render_protagonist` = `0x491ae0`'s
  case-`0x1872d` (`0x491ae0:112-192`).  KEY: part 2 (the body) is byte-identical
  to `FUN_0044d160`/`actor_render_describe` (same clip/mirror/angle/link build +
  all three early-return gates); the arm just wraps it with TWO fixed bank-`0x175`
  cels (frame 0 @ x-256, frame 1 @ x-128) → a 3-cel composite at a 128-px pitch.
  Refactored `actor_emit_part`/`actor_emit_layer` out of `actor_render_static`.
- **Spawn, fully RE'd (commit `08fd0be`):** `0x1872d` is NOT a map object (code
  outside 70000), so `actor_spawn_from_map` never makes it.  Chain: the town intro
  cutscene **`FUN_004d7d80`** (`case 0x334be` = room 210110 / area `0xd2`, gated on
  event flags `0x5f76805`/`0x606aa4f`) → **`FUN_00431d10(0, 0x1872d, anchor=0x65,
  x=0x3200, 0, 0)`** (the by-code `+0x11e0` spawn helper: free-slot scan +
  anchor-relative placement) → **`0x431e30` case-`0x1872d`** which sets layer 9,
  facing `+0x2c`=99, resolves the pos (`0x41ee60`), installs the clip
  (`+0x6c = &DAT_00671c48`), and installs the sprite via **`FUN_00426db0(0, 0x175,
  0, 1, 0, 0, 0)`** — the long-missing `+0x48` FILL PRIMITIVE (`426db0(dir, bank,
  frame_base, b, x_off, mirror_x, y_off)` writes one `actor_sprite_row`; RETIRES
  the ckpt-79 "lazy fill not RE'd" unknown).  `actor_spawn_protagonist` ports the
  end state; `main.c game_actor_walk` dispatches code `0x1872d` →
  `actor_render_protagonist`, else `actor_render_static`.
- **The HORSES fix.**  First render froze the body on frame_base 0 → its rightmost
  cel redrew the wagon-left cel ("the right wagon is cut in half" — USER).  The
  body is the **animated HORSES**.  Decoded the clip `&DAT_00671c48` from the
  user's `sotes.exe` `.rdata` (file off `0x271c48`): base_sprite 2, 4 frames, dur
  18, looping, delta {0,1,2,3} → body cels 2..5 = the horses.  Pointed the
  render-state at a reconstructed `WAGON_CLIP` (the 4 RE'd values) → the body draws
  sprite 2 = the horses.  **USER-CONFIRMED on the feed: "that definitely matches
  retail."**
- **State: 893 pass / 0 fail / 6 skip** (+4).  Ledger **199/194 unchanged** (the
  arm + spawn are bare-VA slices of `0x491ae0`/`0x431e30`; `426db0` referenced by
  bare VA).  quirk #81; PORT-DEBT `actor-protagonist-clip` (body frozen on frame 2;
  the per-tick stepper that trots the horses + the cutscene roll-in are deferred).
  Writeup: `findings/in-game-intro.md` "The 0x1872d SPAWN + the arrival WAGON".
- **NEXT:** (a) the per-tick anim (advance `WAGON_CLIP` so the horses trot — needs
  the `0x46cd70`/`0x54f980` update pass or a minimal sim-tick advance); (b) the
  scripted caravan roll-in + anchor-relative spawn (the `0x4d7d80` cutscene);
  (c) the siblings `0x1872e`/`0x1872f` (likely the CHARACTERS — spawned by
  `0x539e80`/`0x5034b0`); (d) byte-confirm via `render_diff` keyed on res `0x058f`
  vs a panned-camera retail capture.

## Where we are — ckpt 79

**The town actor RENDER CENSUS overturns the ckpt-76/78 "32 static actors"
picture — only 6 of 33 main-band actors DRAW — and the minimal CHARACTER SPAWN
is ported + host-tested.**  This is the gating input the ckpt-77 renderer needed,
captured as ground truth (the methodology's "capture each slot's `+0x48` live").

- **The capture.**  Extended `tools/flow/retail_fields.json` `0x491ae0` with the
  `+0x48` sprite-table reads (`row0_bf` = bank|frame_base dir-0 word, `d1_bf`…
  `d7_bf` for dirs 1-7, `dir_e8`, `alpha_f4`/`skip_284`/`angle_ec`, render-state
  `rs_dstx`/`rs_dsty`/`rs_lo284`) — `thisderef`/`thischain` off the actor ECX —
  and ran a field-spec capture at the town hold (flip 1480/1500/1520,
  `--seed-pin --lockstep --no-turbo`, `trace-retail.jsonl`).  33 actors × 3 flips.
- **The result (corrects quirks #78 + #79).**
  - **27 of the 33 are INVISIBLE** — all-zero `+0x48` in every direction, so
    `FUN_0044d160` returns 0 (`bank==0`).  Collision / trigger / spawn volumes
    (`0x111d6`/`0x112e6`/`0x112e2`/`0x11365`/… — the codes whose `0x431e30` arms
    build a physics body, not a sprite).
  - **Only 6 DRAW** (all dir 0, clip 0 = static, skip 0).  Bank `0x16c` (res
    `0x403`) is the town-OBJECTS sheet → these are static **PROPS, not people-NPCs**
    (USER-confirmed against retail: a fountain + a barrel, at the correct spots):
    `0x1129e`×3 (frame 1, layer 9; a barrel), `0x1129f` (frame 2, layer 9),
    `0x112e5` (frame 36, **layer 10**; the fountain).  The one PERSON is
    **`0x1872d`** the **animated protagonist** (bank `0x175`, clip `0x671c48`,
    `+0x2c`=0x63) — **OUTSIDE** the 70000 CHARACTER range (a SEPARATE spawn), needs
    the `0x491ae0` `0x1872d` multi-part arm; its body-part banks (`0x426`/`0x459`/…)
    are **the bulk of the 36-blit residual**.
  - **`0x426620` ZEROES `+0x48`** (the `type*0x80+0x21c04` in it is the
    **cell-indexed collision-grid** lookup #79 misnamed — it writes `+0x288/+0x28c`,
    not a sprite).  The table is filled **LAZILY** by the state-set machinery
    (`0x40afe0`/`0x41e600`) from a type-keyed def table — **not yet RE'd**.
  - **The props sit at a DETERMINISTIC per-code offset from `map_x*100` — NOT RNG**
    (earlier draft wrongly said "wandered").  All three `0x1129e` share the exact
    `+1800x/+1600y`; positions identical across flips 1480/1500/1520 (static).  The
    visible fountain `0x112e5` is `+0/+0` → lands exactly at `map_x*100` and matches
    retail (USER-confirmed).  Delta source TBD (the `0x426620` alignment arm or the
    lazy fill); the port spawns at `map_x*100`.
  - Census artifact: `/tmp/actor_census.json` (ephemeral; regenerate via the
    field-spec capture above).  Engine-quirk #80; `findings/in-game-intro.md`
    "The town actor RENDER CENSUS".
- **Ported + WIRED + LIVE-VERIFIED (+6 tests, 889 pass).**
  - `src/actor_spawn.{c,h}` (pure): `actor_spawn_from_map(pool, map_data)` = the
    `0x58d460`→`0x431e30` slice — walk the object layers, filter to CHARACTER (code
    70000..79999), activate a slot per object (world `(x,y)*100`, dir 0, layer 9,
    static clip NULL), seed the 3 prop codes' dir-0 sprite rows from the captured
    stand-in (`actor_spawn_sprite_for_code`, PORT-DEBT `actor-sprite-table`), leave
    the rest bank 0.  `actor_spawn_pool` = parallel `{actor, render-state}` arrays.
  - `town_render_step_ex` (town_render.c) adds an **actor seam** (a
    `town_actor_walk_fn` called after the tile walk, before the present, into the
    SAME pool) + passes `present_dims_fn` through; `town_render_step` is the
    tile-only wrapper (existing callers unchanged).
  - `main.c`: `g_actors` populated in `enter_game`; `game_actor_walk` walks it →
    `actor_render_static`; `game_cel_dims` (cel `metric_b8/bc`) = the mode-0 cull
    box; `game_present_blit` `PRESENT_KEYED` arm → `zdd_object_blt_keyed` (`0x5b9b70`).
  - **LIVE-VERIFIED:** port logs `game_actor_walk: 5/32 actors emitted (bank 0x16c
    registered)`; the props render at the correct spots (USER-confirmed on the feed
    — fountain right @ screen ~480, barrel left-edge).  Only `0x112e5` (fountain) is
    fully in-window at the hold (cam 128000); the other props are off-screen-left.
  - Tests: `test_actor_spawn` (5) + `town_render_actor_seam` (1).  Ledger 199/194
    (spawn is a bare-VA slice; only the already-counted `FUN_0044d160` appears).
- **NEXT — the `0x1872d` protagonist multi-part animated arm** (`0x491ae0:112-192`):
  the actual PERSON + the bulk of the 36 residual.  Reuses `anim_clip`; spawned by a
  separate (non-`0x431e30`) path (find it — the protagonist isn't in `g_actors`).
  Then `render_diff` vs retail flip 1500 keyed on `(res, frame)` (camera hold-vs-pan
  is a standing deferral, so the signal is actor-blit IDENTITY).  A fuller prop
  verify can drive the port's pan so the off-screen-left props enter the window.

## Where we are — ckpt 78

**The town actor SPAWN is RE'd end-to-end + byte-verified against the map bytes —
no live drive needed.**  This unblocks the (ckpt-77) ported renderer: the spawn's
*inputs* (per-actor code + world x/y) are now known, parsed by `map_data` already.
Docs-only checkpoint (no C touched; 883 pass unchanged).

- **The chain (corrects the ckpt-76 guess `0x42eb20`/"`0x587e00`'s layer pass"):**
  `0x586010:698` → **`FUN_0058d460`** (room object-population pass) → **`FUN_00431e30`**
  (character activator).  `0x58d460` walks the map descriptor's **86 object-placement
  layers** (`mapobj+0x38` headers ×0x3c + `mapobj+0x3c` sub-ptr records ×0x10) and
  dispatches each by the **range of its type code** (`header+0x10`) into four
  pre-allocated bands off `DAT_008a9b50`, each guarded by a named `"<kind> Object
  Count Over"` abort:  EFFECT 50k→`+0x1160` (`0x41f200`), STRUCTURE 60k→`+0x2560`
  (`0x438a60`), **CHARACTER 70k→`+0x11e0` (`0x431e30`)**, DEVICE 80k→`+0x13e0`
  (`0x557550`).  `0x431e30` (`__thiscall`, ECX=free slot) is a per-type switch:
  sets `+0x1d0=1`/`+0x1d4=type`/`+0xfc=9`(layer)/`+0xe8=0`(dir), zeroes the `+0x48`
  sprite table, stores world (x,y), and a per-type helper (`0x426620` + the
  `0x4264xx–0x4273xx` cluster) installs the sprite/anim from a def table
  (`type*0x80+0x21c04`).
- **The byte-level proof (resolves "codes never assigned as constants"):** the town
  behaviour codes ARE the map object type fields.  `tools/extract/map_data.py …
  --objects` decodes DATA 1022's 86 layers → **15 effect + 39 structure + 32
  character + 0 device**; the 32 character codes + multiplicities are IDENTICAL to
  the ckpt-76 live census (0x112e6 ×10, 0x111d6 ×7, 0x1129e ×3, 0x112e2/0x11365 ×2,
  8 more ×1), with world positions.  The 33rd live actor = the 1 animated NPC
  (`0x1872d`=100141, outside the char range → separate path).
  Proof: `docs/proofs/map-object-layer-format.md`; engine-quirk #79;
  `findings/in-game-intro.md` "The town actor SPAWN".
- **The port input that remains:** the **code → `+0x48` sprite table** mapping (the
  only datum NOT in the map record — `0x431e30`'s per-type def-table install).  RE
  the 13 town codes' cases, OR capture each spawned slot's `+0x48` table live (hook
  `0x431e30` onLeave).  Then a minimal spawn (read the 32 objects from `map_data`,
  fill render-state pos + sprite table + dir + layer 9) drives the ported renderer →
  wire into `game_render` → `render_diff` vs retail flip 1500 (the 36-blit residual
  should drop) → human pixel-verify.

## Where we are — ckpt 77

**The town ACTOR RENDER SIDE is PORTED + host-tested** (the default arm that
draws 32/33 town actors), ahead of the spawn.  Pure, no harness; the SPAWN
(band population — RE'd ckpt 78 above) + the `0x1872d` animated arm + the
`game_render` wiring + pixel-verify are the next arc (needs the human for the feed).

- **Ported (commit `0533603`):**
  - `draw_pool_emit_actor` = **`FUN_00492670`** (`src/draw_pool.c`): the actor
    analog of `draw_pool_emit`; same 0x3c node, mode = `bool(alpha!=0)`, alpha in
    the param8 slot, NULL cel emits nothing.
  - **`src/actor_render.{c,h}` (NEW):** `actor_render_describe` = **`FUN_0044d160`**
    (the static/animated/mirrored/angle sprite descriptor over the per-direction
    table `actor+0x48`) + `actor_render_static` = the **`0x491ae0` default arm**
    (skip flag, layer + override, describe, emit).  actor + render-state are
    LOGICAL structs (the spawn fills them); `actor_sprite_row` (0x14) pinned.
  - `map_present` **MODE 0** (`src/map_present.c`): the opaque-actor keyed path
    (project + cel-dims cull via the new `present_dims_fn` → `PRESENT_KEYED`
    `FUN_005b9b70`).  `dims=NULL` keeps the tile-only contract.
- **Validated:** render-state offsets match the ckpt-76 live `0x491ae0` field spec
  exactly (`rs_x`/`rs_y`/`rs_clip`/`rs_frame` = +4/+8/+0x6c/+0x72); logic
  host-tested bit-exact vs the decompile.  **883 pass / 0 fail / 6 skip** (+18).
  Ledger **199/194** (+`FUN_0044d160`, +`FUN_00492670`).  Both GUI builds clean.
- **NEXT (the gating arc — `findings/in-game-intro.md` "The town ACTOR render side"):**
  1. **The SPAWN** (the `+0x11e0` band activator).  Narrowed: NOT `0x560e60`
     (8 party actors) / NOT `0x584710` (refuted) / NOT the `+0x1d0`+`+0x1d4`
     static writers (`0x456a50` find-by-code, `0x487dc0` cell/collision).  It is
     the **entity subsystem** (`0x42eb20`/`0x4282f0`/`0x429060`/27 KB `0x41f200`)
     processing the **DATA 1022 layer entries** (86 — `map_data` parses them) via
     `FUN_00587e00`'s layer pass.  Empirical pin: `mem_watch --hw` an **ACTIVE**
     slot's `+0x1d4` (only the activation writes it; slot 0 is inert — find an
     active index first via a `0x491ae0` ECX log).
  2. **The `0x1872d` animated arm** (1 actor — a 3-element multi-part descriptor,
     `0x491ae0:112-192`); port WITH the spawn so it pixel-verifies.
  3. **Wire** the band walk into `game_render` (between `map_render_walk` and
     `map_present`) + the Win32 keyed sink + cel-dims callback → `render_diff` vs
     retail flip 1500 (the 36-blit residual should drop).

## Where we are — ckpt 76 (the RE that ckpt 77 built on)

**The town NPC/actor RENDER PATH is RE'd live, the trace tooling is hardened +
documented, and the spawn is narrowed to a precise lead.**  (RE + instrumentation
half of "implement the NPCs"; the render-side port landed ckpt 77 above.)

- **Trace tooling (the user's mandate "harden + document the foundation"):**
  - **`thischain`** field source (`tools/frida/opensummoners-agent.js` `ctReadField`):
    like `chain` but ROOTED at the `__thiscall` `this` (ECX) + pointer hops + `off` —
    reads a field BEHIND a this-pointer (an actor's render-state at `*(actor+0x40)+off`).
    The reusable primitive for probing any entity by its live `this`.
  - **Annotated** `0x491ae0` (actor render entry — behaviour `+0x1d4`, the `+0x48`
    sprite table, render-state pos/clip/frame), `0x560e60` (actor reset → spawn caller
    via `ret_va`), `0x584710` (candidate) in `tools/flow/retail_fields.json`.
- **The actor walk.**  Six actor bands off `DAT_008a9b50`; the **MAIN band is
  `+0x11e0` (0x80=128 slots)**, render-emitted by **`FUN_00491ae0`** (from the per-frame
  driver `0x48c150` free-roam branch) and updated by **`FUN_0054f980`** (from the
  per-tick `0x46cd70`); live when `actor+0x1d0 != 0`.
- **Live trace (retail town hold, flip 1500, `--seed-pin --lockstep`): 33 active
  main-band actors — 32 STATIC** (render-state clip `+0x6c`==0), **1 ANIMATED**
  (`+0x1d4`=`0x1872d`, the protagonist/key NPC).  **32/33 behaviour codes are NOT
  explicit `0x491ae0` cases → they hit the DEFAULT arm `caseD_11257` →
  `FUN_0044d160`** (the static-actor descriptor builder) → the emit tail → **`0x492670`**
  (the actor analog of `draw_pool_emit`; node mode 0=keyed / 1=alpha).  The behaviour
  code drives the **AI** (`0x54f980`, RNG motion deferred ckpt-73), NOT the render —
  **one function (`FUN_0044d160`) renders nearly the whole town.**
- **`FUN_0044d160`** reads `actor+0xe8` (dir) → the per-direction sprite table at
  **`actor+0x48` stride 0x14** (bank/frame_base/x_off/y_off) + the render-state
  (`+0x04/+0x08` world pos, `+0x2c` facing, `+0x6c` clip, `+0x72` frame).  Static actor
  (clip==0): cel = `(bank, frame_base+facing)` at the render-state world pos.
- **Render OUTPUT** (the 36 mode-0 keyed `0x5b9b70` blits @1500): res `0x403`/`0x426`
  (villagers) + `0x459`/`0x462`/`0x46a`/`0x47b`/`0x481`/… — **exactly the ckpt-75
  render_diff residual's named NPC banks**.  These ARE the 36 leftover divergences.
- **The band is a PRE-ALLOCATED 128-slot pool** (`0x586010:476-506` calls
  `FUN_0058cf60(0x40)` 0x80× for the main band; `0x58cf60` zeroes a slot, `+0x1d0=0`).
  So the per-room **spawn = ACTIVATE + configure** a subset, running **after
  `0x586010`'s `"Init Objects"` marker** (`:508`).  The behaviour codes are **data-driven**
  (never literal) → an **entity-by-id** subsystem (ROADMAP `0x420000`); NOT `0x560e60`
  (= the 8 PARTY actors, `ret_va=0x59f578`) / NOT `0x584710` (never fired).
- **State: 865 pass / 0 fail / 6 skip** (no C touched).  Engine-quirk #78;
  PORT-DEBT `present-actor-modes` (render-emit half will land here).
- **NEXT (the implement arc — best as one fresh-context session, ends in pixel verify):**
  1. **Find the `+0x11e0` activator** — instrument the code after `0x586010`'s
     "Init Objects" marker (hook the callee that sets `+0x1d0`/`+0x1d4`/`+0x274`, read
     `ret_va`; or `mem_watch --hw` a slot's `+0x1d0`); cross-ref the map DATA 1022 layer
     entries + ROADMAP `0x42eb20`/`0x4282f0`.
  2. **Port the render side** (pure + host-tested): `FUN_0044d160` (static-prop desc) +
     `FUN_00492670` (node emit) + the `0x491ae0` default-arm tail; the `0x1872d` animated
     arm reuses `anim_clip`.
  3. **Wire** `map_present` modes 0 (keyed `0x5b9b70`) / 1 (alpha `0x5bd550`) — cull dims
     from the sprite (`0x48eac0` mode-0/1 arms) — and drive the actor walk from `game_render`.
  4. **Verify** the town NPC blits vs retail flip 1500 (`render_diff` keyed on `(res,frame)`);
     the ckpt-75 residual should drop from 36.

## Where we are — ckpt 75

**The establishing-shot cinematic LETTERBOX is RE'd, ported, and blit-trace 1:1.**
The single biggest missing layer of the town frame (the ckpt-74 diff's 320 `0x583`
draws) is now drawn.

- **The producer — RE'd from the captured retail blit trace, NOT the `0x5a00c0`
  overlay as ckpt-74 guessed.**  The 320 res-`0x583` blits' return addresses
  (`0x8c48a`/`0x8c4fe` + image base 0x400000 = `0x48c48a`/`0x48c4fe`) land inside
  **`FUN_0048c150`** (the per-frame world driver), lines **124-162** — two
  grid-fill loops AFTER the backdrop present pass (`0x48eac0`).  Loop 1 (`in_ECX+0x44`
  = bottom-bar height, ret `0x48c48a`, emitted first) tiles the cel over dy
  416-476; loop 2 (`in_ECX+0x48` = top-bar height, ret `0x48c4fe`) over dy 0-60.
  Each bar rounds its height up to a multiple of the 4px cel height and tiles at
  64px column pitch (10 cols, dx 0-576; inner loop runs while `(dx+0x80)<0x281`).
  Both heights are **64** for the opening town → the quirk-#74 letterbox.
- **The cel** = main sprite-pool **slot 41** (PE resource **`0x583`**, 64×4, opaque
  `ckey=0x1ffffff`), registered by `ar_register_main_sprites` (extras[] idx 41,
  already run at boot, `main.c:718`).  The engine binds it via `FUN_00418470(0)`
  (the plain frame getter — NO `0x417c40` grade) before the `FUN_005b9a40`
  (`blt_onto`) tile blits.
- **PORTED (pure, host-tested + bit-exact vs the trace): `src/letterbox.{c,h}`** —
  `letterbox_render(top_h, bottom_h, sink, ctx)` ports the two loops verbatim
  (4 tests: the 64/64 town grid bit-exact vs the 320-blit trace, zero bars,
  null sink, the round-up-to-4 arithmetic).  Wired in `main.c`:
  `game_letterbox_blit` resolves `&g_ar_sprite_slots[41]` frame 0 →
  `zdd_object_blt_onto`, called in `game_render` AFTER `town_render_step` (on top of
  the backdrop, matching the engine order).  Heights armed to `LETTERBOX_INTRO_BAR`
  (64) in `enter_game`.
- **VERIFIED two ways.**  (1) `render_diff --retail-frame 1500 --port-frame 1200`:
  the town-frame divergences dropped **356 → 36** — all 320 `0x583` blits now match
  retail on identity + geometry + DDraw state (0 `[rect]`/`[decode]`/`[state]`, 0
  port-extra); the 36 left are exactly the deferred RNG-driven actor/banner/tree
  banks (`present-actor-modes`/`ingame-nontile-layers`).  (2) Port frame 1200
  pixel check: rows 0-63 + 416-479 are `(0,0,0)`, row 64 is the sky band — the
  central 640×352 window.  **USER-CONFIRMED on the feed.**
- **State: 865 pass / 0 fail / 6 skip** (+4).  Ledger **197/192 unchanged**
  (`letterbox.c` is a bare-VA slice of the unported `0x48c150` — no new `FUN_`
  token, correct).  parity-ledger #8.  Engine-quirk #74 updated with the proven
  producer.  PORT-DEBT `ingame-letterbox` (the 64/64 heights stand in for the
  unported `0x5a00c0` cutscene op writing the scene-object `+0x44`/`+0x48`; the
  grid-fill geometry is bit-exact).
- **NEXT chip:** the **"Town of Tonkiness" banner + foreground tree/veg** — the
  `0x5a00c0` scripted-scene overlay player (draw-list `stack+0x98` stride-10;
  caption array `stack+0x3a4` stride 0x124 via font bank `DAT_008a7640`).  Also
  where the pan TRIGGER and the letterbox `+0x44`/`+0x48` writer live — porting it
  closes `ingame-nontile-layers`, the trigger half of `ingame-camera-pan`, and the
  source half of `ingame-letterbox`.  Then the NPC actor render/spawn (entity
  system; RNG-driven motion deferred per ckpt-73).

## Where we are — ckpt 73

**The #75-addendum / ckpt-72 OPEN is RESOLVED: the actor-band residual is the RNG
pillar, and the shared LCG stream is non-deterministic run-to-run EVEN UNDER
`--seed-pin`.**  Ran the ckpt-72 directed live check.

- **Experiment.**  Drove retail TWICE (`--seed-pin --lockstep --no-turbo`, the same
  in-game trace `tests/scenarios/in-game-intro/trace-retail.jsonl`), hooking the
  per-sim-tick actor-update boundary `FUN_0046cd70` and snapshotting the LCG state
  word `DAT_008a4f94` there (new `rng` field in `retail_fields.json`, tagged
  with the deterministic `g_sim_tick`, reset at game_enter).  8644 in-game ticks
  common to both runs.
- **Result: `rng` matches 0/8643 sim-ticks.**  The shared stream is at a
  different phase at *every* in-game tick, despite the pinned seed + the
  deterministic sim-tick index.  (`a0_clip`/`a0_frame` matched 8643/8643 but
  TRIVIALLY — main-band slot 0 `+0x11e0` was inert all run, clip=0/frame=0; the
  `rng` divergence is the real signal.  An animating-actor slot was not
  isolated — a follow-up could re-point the chain to a known NPC slot, but the
  shared-stream result already settles the determinism question.)
- **Mechanism — proven at the anchors, not inferred.**  `prologue_enter`: BOTH runs
  on the IDENTICAL flip 946, yet rng differs (`0x84654e6f` vs `0xa79a2d6e`).  At the
  same flip the engine drew a different *number* of LCG values → a per-PRESENT
  consumer × the non-deterministic presents-per-tick count (quirk #75) desyncs the
  stream phase; it never re-converges.  (newgame_enter A@751 rng 0x6a239b8d / B@750
  rng 0x6a239c54; game_enter A@1432 0x84654e6f / B@1434 0xa79a2d6e.)
- **Why it's the actor band.**  `FUN_0054f980` draws this exact LCG `FUN_005bf505`
  ~40× per tick for idle-wait timers (`+0x5c`), the idle→wander branch pick, and
  wander move-offsets (→ `FUN_00450ef0`) — static two-witness.  A divergent stream →
  different waits/dirs/positions run-to-run = the #75-addendum ~6.7k-px residual.
- **CONCLUSION / the fix.**  An RNG-reading subsystem needs its OWN **RNG anchor**
  (snapshot+restore `DAT_008a4f94` at the game_enter sim-tick, both sides; or re-seed
  the actor RNG per tick) — the camera's `g_sim_tick` anchor is insufficient (it
  works only because the camera reads no RNG).  This makes the #75 "anchor each
  subsystem separately" decision MANDATORY (not optional) for the actor layer.  Port
  bar for the band: **data-1:1 given a matched RNG state** — retail-vs-retail isn't
  observed-1:1 here.  Tooling: `tools/rng_tick_diff.py RUN_A RUN_B`.  Engine-quirk
  #77; `findings/in-game-intro.md`.
- **DIRECTION (user, ckpt 73):** **defer ALL RNG-order parity** — it reaches 1:1
  later, once every in-scene RNG consumer is RE'd and we match consumption order
  (rng+rngcalls per anchor on both sides; the flow trace now carries `rngcalls`,
  the unified consumption signal — committed `4c587c0`).  **Do NOT build the
  actor-RNG anchor now.**  The next chips are **implementing all the VISUAL elements
  of this scene**; RNG-driven behaviour parity comes after the consumer census.
- **NEXT (visual elements, simplest first) — now with the blit trace pinpointing them:**
  0. **The town-frame blit diff (ckpt 74) confirmed the backdrop is pixel-faithful**
     (port 250 blits all matched retail on identity+geometry+state; 0 wrong draws).
     The missing 356 draws ARE the chips below — `render_diff --retail-frame 1500
     --port-frame 1200` names them.
  1. **The establishing-shot overlay = bank `0x583`** — **DONE (ckpt 75, see above):
     the producer is `0x48c150:124-162`, ported as `letterbox.{c,h}`; the 320 blits
     now match retail (356→36 diff).**
  2. **"Town of Tonkiness" banner + foreground tree/veg** — the `0x5a00c0`
     scripted-scene overlay player (draw-list `stack+0x98` stride-10; caption array
     `stack+0x3a4` stride 0x124 via font bank `DAT_008a7640`).  Also where the pan
     TRIGGER lives (`ingame-camera-pan`).
  3. **NPC actor RENDER + spawn** (present modes 0/1/2) — the entity/spawn foundation
     (`0x59f2c0` 8-slot init + `0x560e60`); render the actors even though their
     RNG-driven motion won't be observed-1:1 yet (deferred above).

## Where we are — ckpt 72

**The ACTOR ANIMATION cycle is RE'd end-to-end + the frame-stepper ported — and it
rides the existing sim-tick clock, so there is NO separate counter to pin.**  This
closes the ckpt-71 directed next ("RE the NPC/actor system + its animation cycle,
then pin its counter").

- **The UPDATE chain (per sim-tick), distinct from the render/emit pass.**
  `FUN_00439690:1108` calls `FUN_0046cd70(1)` once per sim-tick (when
  `*(param+0x1c)==0`).  `0x46cd70` is the actor-UPDATE master (not the render walk
  `0x48c150`): it walks the pools off `DAT_008a9b50` (active = `actor+0x1d0!=0`) and
  for the main band (`+0x11e0`, 0x80 slots) calls
  `FUN_0054f980(actor+0x40, actor+0x40, 0, 0)` for the primary render-state entry +
  `(entry-0x294, entry, 1, idx)` for each kinematic sub-entry.
- **`0x54f980` (11597 B) = the per-actor behaviour dispatch on `actor+0x1d4`.** It
  shadow-copies the render-state (the body-part chain), then every animating case
  runs the SAME inline frame-stepper on the render-state anim fields (`+0x6c` clip /
  `+0x70` timer / `+0x72` frame / `+0x74` done): `timer++`; at `>=clip.dur` →
  `frame++`,`timer=0`; at `>=clip.count` → loop (`frame=loop_to`) or one-shot hold
  (`frame=count-1`,`done=1`,`timer=1`).
- **The clip is a fixed 0x154-B 32-frame descriptor** (count@`+0x42`/dur@`+0x44`/
  oneshot@`+0x48`/loop_to@`+0x152`/base@`+0x00`/per-frame delta@`+0x02`/x@`+0x50`/
  y@`+0xd0`) — two witnesses: the stepper + the renderer `0x491ae0` case 0x1872d.
  Clip is (re)set on STATE CHANGE by `0x40afe0`/`0x41e600`, reset-on-change only.
- **PORTED (pure, host-tested bit-exact): `src/anim_clip.{c,h}`** —
  `anim_clip_advance` (the stepper) + `anim_state_set` (the change-gated set) +
  `anim_clip_sprite` (base+delta).  `anim_clip` pins the descriptor layout with
  `_Static_assert`.  **8 tests; 854 pass / 0 fail / 6 skip** (+6).  Both GUI builds
  clean (wildcard picks up `anim_clip.c`; unused for now — actors not yet driven).
- **DETERMINISM CONCLUSION:** `+0x70/+0x72` is a pure function of *(sim-ticks since
  clip-set)* — no GetTickCount/Flip/RNG — so it is already deterministic under the
  camera's `g_sim_tick` anchor (game_enter reset).  No new pin.  This REFINES the
  #75-addendum guess that the anim "reads a counter NOT the camera sim-tick".
- **OPEN (the real residual):** the #75 ~6.7k-px actor-band diff under sim-tick
  matching must be a DIFFERENT pillar — the RNG-driven behaviour (which clip plays /
  position).  `0x54f980`'s idle/wander cases draw the LCG `0x5bf505` for random
  waits + spawn offsets; clip-SET timing is downstream.  ANNOTATED for the check
  (`retail_fields.json` `0x46cd70`/`0x54f980` → `a0_clip/a0_timer/a0_frame`): a live
  capture across two sim-tick-matched runs should show `a0_frame` matching while
  `a0_clip`/position drifts.  Engine-quirk #76; `findings/in-game-intro.md` "The
  actor animation cycle".
- **NEXT:** either (a) live-confirm the above (drive retail twice seed-pinned, read
  the `a0_*` anim fields per sim-tick, diff) → pins the residual to the RNG pillar;
  or (b) move to a named visible layer (the cinematic LETTERBOX quirk #74, or the
  `0x5a00c0` banner/foreground-tree overlay player).

## Where we are — ckpt 70

**The intro-PAN camera is WIRED LIVE — the town backdrop now PANS; the scripted
target-setters ported.**

- **The easer is driven by a live camera in `main.c`.** A static
  `camera_view g_game_camera`: `enter_game` sets `map_w/h` (`dim·0xc80`) + the
  640×480 viewport and `camera_apply_snap(128000, 12800)` (spawn origin =
  `MAP_RENDER_CAM_TOWN_3F2`). `game_render` calls `game_camera_step` each frame:
  the `CALL_TRACE_BEGIN(0x43d1d0)` flow-trace mirror (the X-axis easer state per
  `retail_fields.json`) → `camera_follow_step` → `game_camera_to_mr` projects the
  view onto the `mr_camera` subset → the backdrop renders through the *current*
  scroll (replacing the static const). A hold timer fires the scripted pan.
- **The target-setters ported (`0x439690:599-664`).** `camera_apply_snap`
  (`+0x40` command: clamp tgt to `[0, map-vp]`, cap=0/flag=0, JUMP cur=tgt, zero
  vel — spawn positioning) + `camera_apply_pan` (`+0x4c` command: clamp + set tgt
  / cap=speed / flag=0, leave cur/vel — the easer eases). Host-tested bit-exact (2
  new tests). Referenced `0x439690` by **bare VA** (only the setter slice of the
  8866-B fn is ported — no ledger inflation).
- **Visually confirmed on the feed:** hold (cam x=128000, town right) → mid-pan →
  settled (cam x=12800, town left edge / half-timber house). Pan completes ~400
  frames after the hold timer.
- **Added** `MAP_RENDER_CAM_TOWN_3F2_SETTLED` (x=y=12800) — the determinate
  settled camera both sides share for a flip-anchored full-frame diff with NO
  easer in flight.
- **State (ckpt 70): 848 pass / 0 fail / 6 skip** (+2). Ledger **197/1490 touched
  / 192 tested** (unchanged — easer/shake counted ckpt 69; setters are a bare-VA
  slice of `0x439690`). Both GUI builds clean.
- **CADENCE + TRIGGER MEASURED (ckpt 70b) → the pan is TRAJECTORY-1:1.** A retail
  field-spec trace (`--seed-pin --lockstep --no-turbo`, easer `0x43d1d0` + Flip
  hooked, contiguous Flip whitelist) pinned both: the easer fires **once per 2
  Flips** (the sim runs at half the Flip rate; `cam_x60` is a STEP function,
  −300/2flips cruise) and the pan command fires at **`game_enter + 184` Flips**
  (Flip 1616 HOLD, 1617 PAN). `game_camera_step` now gates the sim to every 2nd
  frame (`hold & 1`); `GAME_CAMERA_HOLD_FRAMES=184` (even, so trigger Flip = sim
  tick). VERIFIED: the port passes through the **identical `cam_x60` sequence** as
  retail (128000,127990,127970,…,−300/2flips — diffed the captured `0x43d1d0`
  mirror). RESIDUAL (PORT-DEBT `ingame-camera-pan`): a ~2-3 Flip startup-jitter
  PHASE (retail's wall-clock sim accumulator — a 4-Flip plateau at 1618-1621 a
  clean 2:1 step can't reproduce; ≤1 step ≈ 3px, transient, zero at hold+settled)
  + the cutscene-script TRIGGER source + the spawn-snap origin derivation — all
  downstream of the in-game sim / `0x5a00c0` port. Writeups:
  `findings/in-game-intro.md` "The camera is WIRED LIVE" + "The pan CADENCE +
  TRIGGER measured".

## Where we are — ckpt 69

**The intro-PAN camera EASER located + ported bit-exact; a HW-watchpoint tool +
the annotation methodology reinforced.**

- **The pan is SCRIPTED** (not leader-follow). Live camera-field probe across the
  establishing shot (`game_enter@1434`→3600): the target x snaps to a FIXED
  **12800** (4 cells, town's left edge) + speed **300** once at hold-end
  (~flip 1617, ~183 flips after entry); Y never moves (`+0x5c`=`+0x70`=12800).
- **The easer = `FUN_0043d1d0`** (called from `0x439690:1123`, before `0x499ab0`
  shake/HUD). Per axis: `dist=|tgt-cur|`; `if vel<dist: cur ±= vel(+far-boost
  when flag&&dist>16000); vel=min(vel+10,cap)`; `else cur=tgt (snap);
  vel=max(vel-10,0)`. cap = `+0x20` (=300). Town pan has flag(`+0x1c`)=0.
- **Found via a HARDWARE WATCHPOINT** — it's dispatched through a heap function
  pointer (invisible to static search). New `tools/mem_watch.py --watch-chain
  ROOTVA:HOPS:OFF:SIZE[:LABEL[:ARM_AT_FLIP]] --hw` resolves the view's heap
  `+0x60` and DR-watches it (frida-17 per-thread API, OpenMare pattern). One run:
  1189 writes, single writer insn `0x43d26d`, trajectory 127970→12800; the
  per-tick deltas 30,40,…,300 pin the formula. (MemoryAccessMonitor livelocks on
  the hot view page — the `--hw` path is the fitting tool.)
- **PORT (pure, host-tested bit-exact):** `src/camera_follow.{c,h}` —
  `camera_follow_axis`/`camera_follow_step` (`FUN_0043d1d0`) + `camera_shake_apply`
  (`FUN_0043d340`). 6 tests validate the captured trajectory, the +10/cap-300
  ramp, exact landing, the flag-gated far-boost, shake-inactive=0. **846 pass / 0
  fail / 6 skip.** Ledger **197/1490 touched / 192 tested** (+`0x43d1d0`,`0x43d340`).
- **ANNOTATED** (the user's directive): a named `camera_follow_step` (0x43d1d0)
  entry in `retail_fields.json` with the view fields incl. the now-known
  `vel_x/vel_y` integrator + the formula; the view struct's fields are named at
  their retail offsets in `camera_follow.h`. Port `CALL_TRACE_BEGIN(0x43d1d0)`
  mirror pending the live-camera wiring.
- **METHODOLOGY (reinforced, CLAUDE.md "Annotate as you RE"):** "annotate" = the
  flow-trace field spec (`retail_fields.json` named functions+fields + port
  `CALL_TRACE_BEGIN` mirrors) — CORE step of finishing any RE/port; thiscall/struct
  tagging is a SEPARATE static-readability lane; never an ad-hoc symbol-rename.
- **NEXT (`ingame-camera-pan`):** wire the stepped `camera_view` into
  `main.c game_render`/`game_drive` (replace the static `MAP_RENDER_CAM_TOWN_3F2`)
  + RE the scripted op that sets tgt=12800/speed=300 at hold-end. Then a
  flip-anchored full-frame backdrop/sky diff is meaningful. Writeup:
  `findings/in-game-intro.md` "The camera EASER located".

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

**Camera is wired + pans (ckpt 70); tiles + 24bpp sky matched (ckpt 68, user-confirmed).**
The smallest visible wins, in order:
0. **PAN backdrop diff DONE — verified pixel-1:1 (ckpt 70b).** Captured fresh retail
   pan frames (`--no-turbo --seed-pin --lockstep`) + their `cam_x60`, matched port
   frames by `cam_x60` (sim'd the port camera → port Flips 1304/1344/1384/1422/1462 ↔
   retail 1617/1660/1700/1740/1780, shared cam 127990/125690/120050/114350/108350),
   diffed: **backdrop Δ0** (shift-search sharp min at dx=dy=0; pan-start x=80 col all
   Δ0). Residual = named missing layers ONLY. Parity-ledger #7. **NEW (quirk #74):** the
   establishing shot is **LETTERBOXED** — black bars rows 0-63 + 416-479 (640×352
   window); the "dark top" the user flagged, plus a matching bottom bar; scene-scoped
   (absent in settled play), likely a `0x5a00c0` overlay.
   **Next chips (named layers, simplest first):** the LETTERBOX (quirk #74), then the
   banner + foreground tree (`0x5a00c0`), then the NPC actors (entity/spawn system).
   The settled-end diff (`MAP_RENDER_CAM_TOWN_3F2_SETTLED`, x=y=12800) remains available
   for a no-cadence-question full-frame check.
1. **The 24bpp parallax colour — DONE (ckpt 68).** Retail grades 24bpp banks at
   **DECODE**, not via the palette: `0x417c40` early-exits to the plain getter for
   a palette-less bank, but its **flag-3 branch** (the 24bpp case) first stamps the
   slot's brightness descriptor (`f_08=1`, scales `f_0c/f_10/f_14`=1000 for tint
   case 0, `f_18`=the LUT base when armed); the lazy `ar_sprite_decode` then runs the
   already-ported `ar_sheet_decode_pixels` (LUT-then-scale, magenta skipped). The
   port's parallax sink used the plain getter and never stamped it →
   `game_arm_parallax_grade()` in `main.c` now replicates the stamp in
   `game_parallax_blit`. Verified raw sky `(66,150,255)`→LUT→565=`(33,125,239)`,
   blue `239` == retail main band; **user-confirmed correct on the feed**. The earlier
   "dark top gradient" was camera/scene-dependent (the unported pan revealing a darker
   sky-texture band), **not** an overlay. (NB the old `(132,186,255)`/`(103,165,231)`
   numbers were wrong — actual raw is `(66,150,255)`.) Remaining: a true row-for-row
   sky `differ_px==0` waits on the intro PAN so the cameras align.
2. **The actor renderers** (`0x491ae0` et al.) → present **modes 0/1/2** (the NPCs).
   **BLOCKED:** `0x491ae0` reads a fully-populated entity object from the actor
   pools off `DAT_008a9b50` — the entity/spawn system isn't ported yet (the upstream
   `0x59f2c0` 8-slot actor init + `FUN_00560e60`). Port that foundation first.
3. **The "Town of Tonkiness" banner + the foreground tree** (`0x5a00c0`, the
   scripted-scene overlay player + the `DAT_008a7640` font bank) — PORT-DEBT
   `ingame-nontile-layers`.
4. **The intro PAN — WIRED LIVE (ckpt 70).** The easer + the `0x439690` target-setters
   are ported and stepped each frame; the town pans hold→settled (feed-confirmed).
   REMAINING (PORT-DEBT `ingame-camera-pan`): the pan TRIGGER (the 183-frame hold timer
   stands in for the `0x5a00c0` cutscene-script op — RE'd together with item 3, the
   scripted-scene overlay player) + the easer step CADENCE (port per-frame vs retail
   per-sim-tick; the −147/flip cruise vs cap-300/tick gap → correlate tick↔flip via a
   live trace before claiming a flip-anchored pan diff mid-flight).

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

How to drive the port in-game live (run INSIDE `nix develop` so
OPENSUMMONERS_GAME_DIR is set — else sotesd.dll fails → BLANK render):
`--input-trace tests/scenarios/in-game-intro/trace-port.jsonl --frames 1400`
(`game_enter@1116`), `--capture-frames "1160,1200,1300"` → BMPs in the game dir →
PNG → feed. CLI file paths (`--input-trace`/`--call-trace`) are now absolutized
before the game-dir chdir (main.c `resolve_launch_path`), so a repo-relative path
Just Works — no need to copy the trace into the game dir, and `--call-trace`
writes to (and logs) the launch CWD. The backdrop renders from `game_enter`.

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
game_drive (the in-game run-loop shell), **anim_clip** (the actor animation cycle:
the per-sim-tick frame-stepper `0x54f980` + clip-set `0x40afe0`/`0x41e600` + the
0x154-B 32-frame clip descriptor — ckpt 72, pure + host-tested; not yet driven,
lands with the actor/entity system), **actor_render** (the town ACTOR render side,
ckpt 77 — `actor_render_describe` = `FUN_0044d160` static-actor descriptor +
`actor_render_static` = the `0x491ae0` default arm; pure + host-tested, not yet
WIRED — blocked on the spawn; the `0x1872d` animated arm + spawn + wiring follow),
**camera_follow** (the per-frame camera
ease-to-target `0x43d1d0` + shake sub-applier `0x43d340` + the `0x439690` SNAP/PAN
target-setters `camera_apply_snap`/`_pan`, ckpt 69-70 — pure + host-tested bit-exact;
**WIRED LIVE into `main.c game_render` ckpt 70** — `g_game_camera` stepped each frame,
the town pans hold→settled), **town_render** (composes the backdrop:
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

## Tooling — Phase B B3 (DDraw blit + state trace) LANDED 2026-06-06
The RENDER drill-in is built + cross-side-verified (`findings/ddraw-blit-trace.md`):
`render_diff.py` names the wrong DRAW (`[sprite]`/`[decode]`/`[rect]`/`[state]`),
`flow_diff.py` names the wrong LOGIC — the two-drill-in coverage requested before NPCs.
- `src/render_id.{c,h}` (host-tested, 7): cel→`(resource_id, frame)` registry (openrecet's
  `tex_name` trick) + `dhash` FNV-1a fingerprint of the DECODED sheet (the improvement: a
  software blitter has CPU pixels at decode → catches RIGHT sprite / WRONG decode). Registered
  at `ar_sprite_slice` (port) / the resolver `0x418470` hook (retail, `installRenderIdHook`).
- Port emits at the 5 blit primitives via `zdd_emit_blit` (`src/zdd.c`); rides the existing
  `call_trace` transport. Retail: blit VAs in `retail_fields.json` + two new agent field
  sources `renderid`/`thisderef` (auto-install, no flag — the `rngcalls` pattern).
- `tools/render_diff.py` (host-tested, 9): aligns each frame's blits by `(va, res, frame)`,
  classifies first divergence; intersection-only field compare (port-only fields never
  false-flag); positional fallback for unnamed cels.
- VERIFIED live (title): retail `res=0x91b` == port `res=0x91b`; ECX/arg reads correct;
  render_diff named all 59 title-phase blits. **Footguns hit:** the port loads `--input-trace`
  BEFORE the game-dir chdir (use an ABSOLUTE path), and `--call-trace` opens its file in the
  launcher CWD (not the game dir); run the launcher INSIDE `nix develop` or sotesd.dll fails
  to load (err 126 → blank render). **Next layers:** retail decode-hash (so `[decode]` fires
  cross-side), the cdecl `0x5bd550` retail spec, a same-scene aligned in-game diff.

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
