# Session handoff — last updated 2026-06-02 (8d fully ported + wired live, ckpt 25)

> **ckpt 25:** **8d — the genuine sprite pixel source — is fully ported,
> host-tested, and wired into the live drive.** Three layers in `zdd.c`
> (`zdd_object_new_cell`/`_build_cell`/`_copy_cell_pixels` = FUN_005b9280/
> _9630/_9910), four format converters in `bitmap_session.c`
> (`bs_convert_to_16bpp`/`_8bpp_to_24bpp`/`_24bpp_to_32bpp` +
> `bs_load_palette_from` = 0x5b7310/_74f0/_7270/_7bd0), and the full slicer
> body `ar_sprite_slice` (FUN_004188b0: trim-scan + format switch + build).
> `main.c` adapters bind `ar_frame_build_hook`/`ar_frame_free_hook`/
> **new `ar_sheet_format_hook`** to the live ZDD. **LIVE (self-serviced
> Frida-class): boots windowed at depth=16bpp, drives the title, flips, zero
> DDERR, zero crashes through 900 frames.** 647 host tests (0 fail, 6 skip;
> +17); ledger **138/1490 (8.6%), 135 tested**. Corrected findings →
> engine-quirks **#49** (format converters live in the slicer, not the pixel
> writer) + **#50** (slicer passes cell_w/cell_h as the trim scanner's
> height/width).
>
> ⚠ **8d fires but produces no *visible* sprites yet — and that's NOT an 8d
> bug.** The title banks (pool 19/20) are never **registered** at boot, so
> `ar_sprite_slot_frame` returns NULL (entries==NULL) and the
> decode→slice→8d chain never runs. See **Next move #1**.


**This is the first thing to read at the start of every session.**

Rolling state — REWRITE on each meaningful checkpoint, don't append.
`docs/PROGRESS.md` is the append-only changelog; this file is "where
to pick up *right now*".

## ⭐ Current state (ckpt 25): the whole render pipeline is ported; only bank registration is missing

Everything from the title scene runner down to the per-cell DDraw surface is
ported, host-tested, and live-crash-clean. The chain that runs every frame:

```
title_scene_step  → render sink (title_sink) → resolve_frame(bank 19/20)
   → ar_sprite_slot_frame(slot, id)
        → [slot unregistered ⇒ entries==NULL ⇒ returns NULL]   ← THE GAP
        → ar_sprite_decode_hook → ar_sprite_decode (0x4184a0)
             → bs_decode_resource (needs slot->settings = PE resource source)
             → ar_sprite_slice (0x4188b0): trim-scan + format switch + build
                  → ar_sheet_format_hook → bs_convert_to_16bpp (RGB565)
                  → ar_frame_build_hook → zdd_object_new_cell (8d) → real surface
```

**8d (the bottom three layers) is DONE.** What's missing is the **top**: the
title banks at pool indices 19/20 (`AR_SPR_TITLE_MAIN`/`_CURSOR`) are never
registered, so the getter short-circuits to NULL and nothing downstream runs.
`ar_register_main_sprites(zdd, group, settings, sotesp_module)`
(`src/asset_register.c:1295`) is the registrar — it stamps slots 1..9 with
hard-coded resource IDs (0x49f, 0x448, 0x4a2, …) — **but it needs a valid
`settings` record (the launcher's PE-resource source object) + the sotesp.dll
HMODULE**, neither of which the drop-in builds yet. That is the launcher /
asset-registration subsystem (milestone-4-adjacent), **the real next move**.

Also still deferred (validate once sprites flow): the sink's `LOGO`/`SPARKLE`/
`MENU_CURSOR` arms are no-op (alpha-ramp draws, ctx callbacks); the cold-boot
intro must reach phase 8 (menu) for banked sprites to be drawn at all.

## (ckpt 22, still true): the title scene is driven from `main.c` — the loop runs live

`src/title_drive.{c,h}` is the **caller side of `FUN_0056aea0`** (the plumbing
its retail caller `FUN_00562ea0` owns), and `main.c` now uses it as its actual
per-frame loop. So the milestone-0 runner — a tested-in-isolation unit since
ckpt 11 — is finally **wired into the drop-in and bound to the live ZDD.**

- **`title_drive_init`** allocates the scene's object graph (a `sel_list`
  menu-tree owner + its 0x1b0 `menu_node` at `entry[0]` = the slot the lazy
  phase-8 `title_menu_spawn` configures; and the `input_mgr` with a
  **fully-populated idle ring** — the poll/scan paths deref `ring[i]` with no
  NULL guard, so every slot must point at a real record), binds the render sink
  (`title_sink_ctx` → `title_render_sink_hook = title_render_sink`), and zeroes
  the FSM (`title_scene_init`). `title_drive_step` runs one `title_scene_step`
  and latches the result on `TITLE_SCENE_DONE`. `title_drive_shutdown` unbinds +
  frees (mirrors the test harness's `free_spawn`).
- **`main.c`**: after DDraw init in mode 2, `init_title_drive` binds the sink to
  `g_zdd->primary_obj` with `drive_present` (→ `zdd_present`) + `drive_log_flip`
  thunks, installs `ar_sprite_decode_hook = ar_sprite_decode`, allocates the
  drive. `main_loop_body` runs one `title_scene_step` per iteration; a render
  iteration presents via `TITLE_DRAW_FLIP`; scene completion logs the result +
  stops. **`--no-title-scene`** falls back to the legacy minimal present loop.

**8d (`0x5b9280`, `ar_frame_build_hook`) is now ported + wired (ckpt 25)** but
sprites still resolve NULL because the banks are unregistered (the getter
short-circuits before the build hook). So the scene currently still renders a
**cleared + flipped window with no sprites** — register the banks (Next move #1)
to light it up. Live: window comes up, zero DDERR, flips at 60fps, no crash.

**Sink bank resolution (durable, ckpt 21, render half `0x56bb04..0x56bf1a`):**
every per-phase draw resolves its source frame out of ONE of two fixed sprite
banks, then blits onto `DAT_008a93cc->[0x16c]` (= `g_zdd->primary_obj`, +0x16c):
- **MAIN bank** = `0x8a7658` = **pool slot 19** (`ar_pool_get_slot(19)`) →
  `AR_SPR_TITLE_MAIN`. Logos (frames 1/2), press-button (2/3/4), sparkle (4/5),
  menu bg/sprite (5/6). The cmd `asset` is the `ar_sprite_slot_frame` frame id.
- **CURSOR bank** = `0x8a765c` = **pool slot 20** → `AR_SPR_TITLE_CURSOR`. The
  menu-selection highlight; frame id = the selected row index.

Sink arms: `SURFACE_RESET`/`SURFACE_CLEAR`/`SPRITE`/`SPRITE_LEVEL`/`FRAME_END`/
`FLIP`/`LOG_FLIPPING` faithful + host-tested (the whole intro + menu-bg +
fade-out path). `LOGO`/`SPARKLE`/`MENU_CURSOR` deferred behind ctx callbacks
(no-op default) — alpha-ramp draws whose blend-descriptor *pointer* can't
round-trip the 32-bit `alpha` field on a 64-bit host, and which only fire once
the run-time ramps (`0x8a92b8`/`0x8a9308`) are populated (never at a cold boot,
so no intro/menu-bg fidelity cost). **Wire + validate these against live goldens
during the 8d/live-verify chip** (the command stream may need the sparkle
src-rect + cursor numerator added). Full sink detail: `src/title_sink.h`.

**Note:** the per-cell DDraw surface builder **8d** (`0x5b9280`,
`ar_frame_build_hook`) is now **ported + wired** (ckpt 25) — the build hook is
`title_frame_build` → `zdd_object_new_cell`. It is live-crash-clean but does
not fire yet (banks unregistered; see "Current state" above).

## (ckpt 13): TAS framework — retail ground-truth capture is live

Mirrors openrecet's TAS harness. **Two new self-serviceable Frida
capabilities, both validated live**, give us deterministic retail ground
truth (the port can't render yet, so retail is where ground truth lives):

1. **Frame capture** — `tools/run-retail.sh --no-turbo --capture-frames
   "60,200,…"` dumps `runs/<dir>/frames/frame_NNNNN.png` (640×480 lossless).
   Walks `*(0x8a93cc)->[0x16c](paint_ctx)->[0x2c]` to the DDraw surface,
   GetDC+BitBlt→24bpp DIB→PNG, at the Flip hook.
2. **Input injection** — `--input-trace <file.jsonl>` replays a sparse
   `{frame, ids}` trace into the engine's input ring (hidden window ⇒ DInput
   is silent ⇒ ring is ours). Hooks the poll consumer `0x43c110` per scene.

**Validated:** a scripted trace clicks the whole **NEW GAME** path — title
`Start` → difficulty config menu → DOWN×2 → `Start Game` → confirm → Elemental
Stone intro → prologue narration — fully deterministic, captured frame-for-
frame. See `docs/findings/new-game-flow.md`, `docs/plans/tas-framework.md`.

**Button ids (engine-quirks #42/#43 — input.md was wrong):** up = id 1,
**down = id 3**, confirm = id 0x24 (ids 2/4 are page up/down = no-ops in
single-column menus). Each scene has its **own** input-manager — inject into
the current poll's `ecx`, never a cached one.

> ⚠ **Always launch retail via `tools/run-retail.sh`, never `frida_capture.py`
> directly** — the default `vendor/original/sotes.exe` is the Steam-DRM-packed
> image (spawning it stalls + leaves an orphan game window). `run-retail.sh`
> drops the unpacked exe next to the DLLs and kills the child on exit.

**Remaining harness gaps:** the prologue→first-playable-map (a timed cutscene;
best from a **recorded human trace** distilled to sparse, or RE the prologue
sequencer); ~~port-side `input_trace.{c,h}`~~ **DONE ckpt 24** (`--input-trace`);
port frame capture (still blocked on milestone-0 rendering / 8d pixels).

## Where we are

**Milestone 0 (title screen renders) — `FUN_0056aea0` is one running unit
and its update half is now complete.** Ckpts 1–10 ported the pieces (fade
FSM, pacing FSM, menu spawn, per-frame input dispatch, render step,
teardown); ckpt 11 wired them into the orchestrator that *is* the outer
`do { … } while(1)` body (`0x56b002..0x56ba75`): **`title_scene_step` /
`title_scene_init`** in `src/title_scene.c`. **Checkpoint 12 (this session)
ported the last deferred slice of the update half — the skip-splash
early-out.**

One call = one loop iteration: `title_pace_step` (pump fires through a hook)
→ on a `TITLE_PACE_RENDER` iteration `title_render_step` draws+presents and
loops; on a `TITLE_PACE_UPDATE` iteration the update half (pre-update side
effects → the `0x22` abort poll → **the skip-splash early-out** →
`title_fade_step`, with `title_menu_spawn` on first menu entry +
`title_menu_input_step`, and `title_menu_teardown` before the phase-10
fade-out → the per-frame tail). Every still-unported per-frame engine call
(`0x5b1030` pump, `0x43e140`/`0x40fe00`/`0x566250` pre, `0x56c930` post,
`0x43c2e0` per-entry, BGM SetNextSegment, `0x56c070` sparkle) routes through
a nullable **`title_scene_hooks`** struct; menu-input side effects keep using
the `title_menu_*_hook` globals; render bridges keep using
`title_render_sink_hook`. So the whole milestone-0 control flow is one
composable, testable unit — **no engine subsystem pulled in.**

**Skip-splash early-out (ckpt 12, `0x56b0e8..0x56b150`):** a fresh button
press during the intro jumps straight to the menu. In `title_scene_step`
just after the `0x22` abort poll: `input_any_fresh_press` scans the ring; on
a hit it zeros the fade, fires the BGM `SetNextSegment` cue (when still
before phase 3, reusing the `set_next_segment` hook), flushes the ring + axis
state (`input_mgr_reset`), and forces phase 8. Gated on the scene's new
`skip_intro` field (`param_1`): clear ⇒ a press is ignored at phase 0 (first
boot plays the studio fade in full); set ⇒ it skips from phase 0 too; phases
1..7 always skip on a press. The flush mapped the input-mgr past the ring:
the two axis-held flags (`+0x114`/`+0x118`) are `axis_held[0]/[1]` of an
**11-dword array A**, with a parallel **array B at `+0x140`** + `+0x10c`/
`+0x110`/`+0x16c` scalars — all modeled in `input_mgr` (offsets pinned by
`_Static_assert`). New finding → **engine-quirks #41**. Left out: retail's
scene-local sparkle-counter reset (`var_3eh_2`) — part of the deferred
sparkle-trail subsystem, not the runner.

**Anatomy (from ckpt 11):** the scene returns *only* out of the update half
(the `0x22` abort poll → result 6, or the phase-10 fade-out completing →
result = committed action / 0 on watchdog); the **render half never returns,
it loops**. The idle watchdog (`local_50`) increments on *every* update frame
in *all* phases. The menu spawns *and* runs its first input poll on the same
frame (gate still closed → first menu frame can't latch).

**542 host tests pass, 0 fail, 6 skip (of 548)** — 8 new this ckpt (input
`any_fresh_press` match/empty/flag+age, `input_mgr_reset` flush; skip-splash
jump-to-menu with fade reset + ring flush, BGM-cue gating, the phase-0
`skip_intro` gate both ways, the no-press no-op). Both 32-bit cross-builds
clean. Ledger **122/1490 touched (7.5%), 119 tested** (unchanged — the
early-out is a slice of the already-counted `FUN_0056aea0`; its new input
helpers reference the slice by bare VA).

**Orientation docs (read for the bigger picture):**

- `docs/STATUS.md` — coverage headline (DERIVED). 122/1490 touched, 9.7%
  of bytes, 119 host-tested.
- `docs/ROADMAP.md` — 11-milestone order + subsystem map + port-readiness.
- `docs/findings/title-scene.md` — the title runner's full anatomy; the
  input-dispatch section now has the ckpt-11 orchestrator notes.
- `docs/findings/menu-list.md` — the menu controller: scroll/nav/latch,
  geometry ctor/dtor, the grid-cell finalizer, the menu-node builder + tree.
- `docs/findings/input.md` — the input ring + poll; only the ring
  *producer* remains black-box (it also fills `+0x114/+0x118`).
- `docs/port-frontier.md` — DERIVED "what to port next".
- `findings/engine-quirks.md` #15–#40.

**Tooling (run after every port that lands):**

```
python3 tools/gen_port_ledger.py && python3 tools/gen_frontier.py
```

**Static disasm:** the canonical unpacked image for r2 is
`vendor/unpacked/sotes.unpacked.exe` (the `vendor/original/sotes.exe`
symlink target is the **packed** Steam build — its `.text` is encrypted, so
r2 reads garbage there). Recipe:
`nix develop --command bash -c "r2 -q -e scr.color=0 -c 'af @ <va>; pdf @ <va>' vendor/unpacked/sotes.unpacked.exe"`.
For a jump table: `pxw <n> @ <table-va>`.

**Structural-parity harness — LIVE-VERIFIED 2026-06-02 (ckpt 12).** How-to:
`docs/parity-harness.md`; design: `docs/plans/parity-harness.md`. The
call-trace machinery now **works end-to-end against live retail**: a no-turbo
capture with the *full* 1743-VA candidate set hooked booted retail to its
title and emitted **1.8M events over 1914 Flip frames, zero crashes** — so
**the bisect step is unnecessary** (`engine_vas_frida_safe.json` written
directly = the full set). Frida is **always up + UAC auto-approved** on this
host → the harness is **self-serviceable, not a human gate**
([[reference_frida]]). Two hard live findings: (a) **everything live must be
`--no-turbo`** — turbo freezes the splash before the pump (quirk #29:
msg_count/Flips = 0); no-turbo renders normally. (b) The retail capture
already **confirmed the next render-bridge target**: `0x5bd680` (software
alpha blitter) + `0x5bd550` are the hot per-frame draw primitive (4279
calls), and the per-frame counts validate the ported pacing split
(`0x56c930` post-update ≈ half of `0x56c180` compose / `0x5b8fc0` Flip). The
**call_trace_diff is still blocked** only on the *port* side: `main.c` must
drive `title_scene_step` before `opensummoners.exe --call-trace` produces a
comparable trace (Next move #2). `mem_watch.py` (input-ring producer) is the
remaining live probe not yet exercised.

NB: only put a `FUN_<va>` token in `src/` for a function you have
actually ported — the ledger generator treats any `FUN_<va>` in src as a
port signal. Reference *unported* callees by bare VA (`0x494e10`,
`0x56c610`), not `FUN_...`, or you'll inflate the headline. **Always regen
the ledger and check the count after a port** (ckpt 11 bit us again: a
single `FUN_0056bfd0` in a header comment bumped 122→123 until corrected to
the bare VA).

## Module inventory (13 modules)

Pixel-Drawer, Asset-Register, Bitmap-Session, WndProc, ZDD wrapper,
cs_dispatch, app_pump, title_scene (`FUN_0056aea0` — **fully ported and
wired**: fade FSM + pacing FSM + update half + render half + the
**`title_scene_step` orchestrator** = the whole outer loop), input
(`FUN_0043c110`), obj_container (`FUN_00412c10` + `FUN_00414080`), menu_list
(`FUN_004192b0` + `FUN_0043ca40` + `FUN_0043ce50` + `FUN_0040f5c0` +
`FUN_0040e0c0` + `FUN_00411f40` + `FUN_0040f3e0`), **title_render** (the
render bridge, ckpt 17–19: `title_compositor_draw` `FUN_0056c180` + wrappers
`title_draw_sprite`/`_level`/`_menu_cursor`/`_sparkle` = `0x56c610`/`_4e0`/
`_470`/`_580`; pairs with `zdd_object_blt_clipped` `FUN_005b9bf0` in zdd)),
**title_sink** (ckpt 21: `title_render_sink` — the cmd-stream→ZDD-blit bridge
resolving banks 19/20, behind `title_render_sink_hook`), and **title_drive**
(ckpt 22: `title_drive_{init,step,shutdown}` — the caller side of
`FUN_0056aea0`: owns the scene object graph, binds the sink, runs the loop).
**The runner is driven by `main.c`** (ckpt 22) with the sink bound to the live
primary, and **8d is wired** (ckpt 25: `title_frame_build`/`_free`/
`title_sheet_format` adapters → `ar_frame_build_hook`/`_free_hook`/
`ar_sheet_format_hook`). **Live-verified ckpt 25:** windowed 16bpp, 0 DDERR,
60fps, 0 crashes through 900 frames — renders a cleared+flipped window with no
sprites still (banks unregistered, Next move #1). `--no-title-scene` restores
the legacy minimal `main_loop_body`.

## Active goal

**Direction set by the user at ckpt 13: make the PORT render the scenes the
new-game trace already covers — title menu + new-game menus + the prologue
(stone/narration) — to 1:1-match retail, using the harness-captured goldens
as the pixel target. Do NOT extend the trace toward in-game yet; "once we
have prologue and main menu rendering we extend the trace."**

So the work is the milestone-0 render port (move #1) + driving the runner
(move #2), now with concrete golden frames to diff against (the harness is the
yardstick). `FUN_0056aea0` is fully ported/composed/update-complete; what's
missing is real pixels: the draw bridges behind the render sink, and driving
the runner from the drop-in.

**Render-port task list (ckpt 13):** ~~(7) port the software alpha blitter
`0x5bd680`~~ **DONE ckpt 14** (`zdd_alpha_blit`/`_pixels`, commit `cd95935`).
~~(8) blit orchestrator `0x5bd550`~~ **DONE ckpt 15** (`zdd_blit_orchestrate` +
`zdd_object_blt_rects`; complex path proven dead, quirk #45).
~~(8a) sprite frame getter `0x418470`~~ **DONE ckpt 16** (`ar_sprite_slot_frame`).
~~(8b) the compositor `0x56c180` + the wrappers `0x56c470`/`_4e0`/`_580`/`_610`
(+ `0x5b9bf0`)~~ **DONE ckpt 17–19** — the whole `title_render` module +
`zdd_object_blt_clipped`. ~~(8c) the sprite-sheet decoder `0x4184a0` + slicer
`0x4188b0`~~ **DONE ckpt 20** (`ar_sprite_decode` / `ar_sprite_slice` /
`ar_sheet_decode_pixels`; the resource/DIB layer was already `bitmap_session`).
**The remaining render-side chips, in dependency order (decoded in
`docs/findings/sprite-pipeline.md`):**
- ~~**(8d) the per-cell DDraw surface builder `0x5b9280`**~~ **DONE ckpt 25**
  (`zdd_object_new_cell`/`_build_cell`/`_copy_cell_pixels` + the four
  converters `bs_convert_*` + the full slicer `ar_sprite_slice` + the `main.c`
  hook adapters). Live-crash-clean at 16bpp; does not fire yet (banks
  unregistered — see "Current state" + Next move #1).
- ~~**(9a) implement `title_render_sink`**~~ **DONE ckpt 21** (`src/title_sink.
  {c,h}`). Resolves banks 19/20, faithful for the intro + menu-bg + fade path;
  LOGO/SPARKLE/MENU_CURSOR deferred behind ctx callbacks (still to wire).
- ~~**(9b) drive `title_scene_step` from `main.c`**~~ **DONE ckpt 22**
  (`src/title_drive.{c,h}` + `main.c`). Live-verified ckpt 25 (60fps, 0 DDERR).
- **(10)** port-side frame capture + a `push_comparison.py` (port|retail
  amplified diff to llm-feed) to close the pixel-parity loop against the goldens
  in `runs/title-idle` & `runs/newgame-full`. **Now gated on bank registration**
  (Next move #1), not on 8d — 8d is done.

## Next move (pick one — recommendation first)

> Context (ckpt 25): the **entire render pipeline is ported** — title runner →
> sink → compositor → blit primitives → 8d per-cell surface builder → format
> converters. The drive runs live, windowed at 16bpp, zero DDERR/crashes. The
> ONE thing standing between "blank window" and "the title screen renders" is
> **sprite-bank registration**: the getter returns NULL because pool slots
> 19/20 are never registered. Wire registration → the whole chain lights up and
> 8d's pixels finally hit the screen. This is the payoff move.

1. **(recommended) Register the title sprite banks so the pipeline lights up.**
   `ar_register_main_sprites(zdd, group, settings, sotesp_module)`
   (`asset_register.c:1295`) stamps pool slots 1..9 with the title resource IDs
   (0x49f logo, 0x448 press-button, 0x4a2, 0x49d/0x913/0x91b backgrounds,
   0x91c/0x91d, 0x8df). **The hard part is the args:**
   - `settings` = the launcher's **PE-resource source record** (the object
     `bs_decode_resource` dereferences as the HMODULE/resource dir). The
     drop-in doesn't build this yet — it's the launcher/config subsystem
     (`config.dat`, `FUN_005a4770`, milestone 4). **RE what `settings` must
     point at** (likely just an HMODULE for `LoadResource`/`FindResource` on
     the game exe or sotesp.dll) — it may be far simpler than full config
     parsing. Start by disassembling `bs_load_pe_resource`'s real use of the
     settings pointer + a retail call to `ar_register_main_sprites`
     (FUN_005749b0) to see how `settings`/`sotesp_module` are sourced.
   - Map pool index 19/20 (`AR_SPR_TITLE_MAIN`/`_CURSOR`) vs the registrar's
     slots 1..9 — confirm which registrar populates the banks the sink reads
     (`ar_pool_get_slot(19)`); there may be an index-base offset.
   Then: live-run, confirm `ar_sprite_decode` fires (add a temp log), 8d builds
   surfaces, and the title logo/menu actually blits. **Frida self-serviceable
   ([[reference_frida]]); use a recorded log or `--input-trace` to reach the
   menu phase.** Once pixels flow, validate against the harness goldens
   (`runs/title-idle`) and wire the deferred sink arms (LOGO/SPARKLE/
   MENU_CURSOR) + the port-side frame capture (10).

2. **Live-validate the `--input-trace` injection** (ckpt 24, still unverified):
   does an injected press actually skip the splash / nav the menu on the port?
   Best done together with #1 (you need the menu phase reached anyway). Frida
   self-serviceable, no-turbo.

3. **`mem_watch.py` on the input-ring producer** — the one live probe not yet
   exercised. `mem_watch.py --region <+0x108 addr>:64:input_ring` (no-turbo!)
   to catch the DInput `GetDeviceState` writer (vtable `[0x24]`) that fills the
   ring + the `axis_held` arrays (quirk #41) — the last input black box. Needs
   the runtime address of the input-manager object first (resolve via a hook on
   a known consumer like `0x43c110`). Self-serviceable now ([[reference_frida]]).

## Open RE threads (see ROADMAP subsystem map for the rest)

- **`FUN_0056aea0`** title scene runner — **fully ported + wired + update
  half complete + driven from `main.c` (ckpt 22), live-verified (ckpt 25).**
  The whole render pipeline below it is ported too; remaining is upstream bank
  registration (Next move #1), not the runner or the pixel path.
- **Render-half draw bridges** (will sit behind `title_render_sink_hook`).
  **The whole blit-primitive layer is ported** in `zdd.c`: `0x5b9410`
  (surface reset = `zdd_object_clear`), `0x5b9b70` (color-key blit/clear =
  `zdd_object_blt_keyed`), `0x5b8fc0` (Flip = `zdd_present`), `0x5bd680`
  (alpha core, ckpt 14), `0x5bd550` (orchestrator = `zdd_blit_orchestrate`,
  ckpt 15), `0x5b9ae0` (`zdd_object_blt_rects`, ckpt 15, dead-path only),
  `0x5b9bf0` (`zdd_object_blt_clipped`, ckpt 19). The sprite frame getter
  `0x418470` (`ar_sprite_slot_frame`, ckpt 16) + **the whole compositor +
  wrapper layer (ckpt 17–19) are ported** in `title_render` (`0x56c180`/
  `0x56c610`/`_4e0`/`_470`/`_580`) + **the sheet decoder `0x4184a0` + slicer
  `0x4188b0` (ckpt 20)** in `asset_register.c` (`ar_sprite_decode` /
  `ar_sprite_slice` / `ar_sheet_decode_pixels`). **8d — the per-cell DDraw
  surface builder `0x5b9280` — is now ported too (ckpt 25)** (`zdd_object_
  new_cell`/`_build_cell`/`_copy_cell_pixels` + `ar_frame_free_hook`'s
  `0x5b9390` release + `0x5b6f80` trim metadata + the `0x5b7310/_74f0/_7270`
  format converters + `0x5b7bd0` palette, all wired). Only the logo `0x494e10`
  remains on this layer. The asset pool
  `DAT_008a760c` is the already-modeled `ar_sprite_slot` pool
  (`ar_pool_get_slot`). The
  ramps `0x8a92b8`/`0x8a9308` are pixel_drawer's `g_pd_boot_group_a/_b` as
  pointer tables; the wrappers/compositor take them (+ the primary surface)
  as params today — the sink will supply the real globals.
- **Outer-loop side effects** (stubbed behind `title_scene_hooks`):
  `0x5b1030` (pump), `0x43e140`/`0x40fe00`/`0x566250` (pre-update),
  `0x56c930` (post-update), `0x43c2e0` (per-owner-entry). Port when their
  subsystems come up.
- **`0x40fa00`** the cell text-layout / glyph builder (800 B; SJIS parse,
  `#`-colour escapes, font-metric table) — its own text subsystem;
  `menu_row_finalize` calls it via a hook until it lands.
- **SFX `0x411390`** + **joystick `0x5ba120/_290`** + **save-data notify
  `0x41bb80`** + **watchdog `0x40a5d0`** — the four `title_menu_input_step`
  side effects; port when their subsystems come up (audio = milestone 3;
  DInput pad attach; god-object dispatch).
- **Input** poll + latch + nav + skip-splash scan/flush **DONE**. The
  input-mgr model now covers the ring + the two 11-dword arrays at `+0x114`
  (axis_held, [0]=V [1]=H) / `+0x140` + `+0x10c`/`+0x110`/`+0x16c` (quirk #41).
  Remaining: the **producer** that fills the ring (DInput `GetDeviceState`,
  vtable `[0x24]`) and the axis-held flags — black box; `mem_watch.py` is the
  tool. Array B (`+0x140`) semantics also still unknown.
- **Audio ZDM** `FUN_005bab10`/`_5bc150` + SFX player `FUN_00411390` —
  milestone 3 (WMF/COM, hard).
- **Launcher `config.dat`** `FUN_005a4770` (46 KB) — milestone 4.
- **Hash-id asset directory** `FUN_00556eb0` — recover the ID→name table.
- God-object `DAT_008a9b50`/`DAT_008a6e80` layout (engine-quirks #15) —
  model as we go (the render half reads `DAT_008a93cc->[0x16c]` = primary
  surface, `DAT_008a93ec` = hWnd, `0x8a9308` = the 20-dword alpha ramp).
- Frida turbo: add `GetTickCount` + `WaitMessage` hooks to the agent
  (quirk #29 explains why turbo currently freezes the splash).

## How to apply

When the user says "continue RE work" (or similar):

1. Read this file first, then `STATUS.md` + `ROADMAP.md`.
2. Pick the recommended next move (or whichever the user redirects to).
3. Work port-and-test style: small unit → tests → commit. Each ported
   function gets a `FUN_XXXXXX` provenance comment (the ledger keys on it)
   and a test spot-checking behaviour vs hand-computed expectations. Pin
   retail struct offsets via `_Static_assert` guarded by
   `#if UINTPTR_MAX == 0xFFFFFFFFu`. For a Ghidra-unrecovered jump table,
   recover it in r2 first (`pxw <n> @ <table-va>`). When a decompiled branch
   or a `__thiscall` arg list looks contradictory, **disassemble it** (r2
   `pdf`) to resolve.
4. **Append any engine quirk** you find to `findings/engine-quirks.md`.
5. **Regenerate the derived artifacts** (`gen_port_ledger.py` +
   `gen_frontier.py`) when a port lands — and **check the headline count
   didn't move unexpectedly** (a stray `FUN_` for an unported callee inflates
   it; reference unported callees by bare VA).
6. Update THIS file at each meaningful checkpoint; append to PROGRESS.md.
7. **Suggest a `/clear`** at the natural stop point — the docs are the
   durable memory, not context.
