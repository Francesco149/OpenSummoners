# Session handoff — last updated 2026-06-02 (title render sink ported, ckpt 21)

**This is the first thing to read at the start of every session.**

Rolling state — REWRITE on each meaningful checkpoint, don't append.
`docs/PROGRESS.md` is the append-only changelog; this file is "where
to pick up *right now*".

## ⭐ NEW (ckpt 21): the title render sink is ported — the cmd stream now drives real ZDD blits

`src/title_sink.{c,h}` (the `ar_pool`/`zdd` bridge) turns
`title_render_step`'s abstract `TITLE_DRAW_*` command stream (the render half
of `FUN_0056aea0`, emitted through `title_render_sink_hook`) into the retail
render half's actual ZDD calls, against a **bound** primary surface. Install
with `title_sink_bind(&ctx); title_render_sink_hook = title_render_sink;`.
This is the **sink** half of Next move #1 — **the drive half (wire it from
`main.c`) is the next chip, ckpt 22.**

**RE landed this ckpt** (render half `0x56bb04..0x56bf1a`, r2): every per-phase
draw resolves its source frame out of ONE of two fixed sprite banks, then blits
onto `DAT_008a93cc->[0x16c]` (= the ZDD `primary_obj` at +0x16c, i.e.
`g_zdd->primary_obj`):
- **MAIN bank** = `0x8a7658` = **pool slot 19** (`ar_pool_get_slot(19)`; main
  pool base `0x8a7640` + 6·4) → `AR_SPR_TITLE_MAIN`. Logos (frames 1/2),
  press-button (2/3/4), sparkle (4/5), menu bg/sprite (5/6). The cmd `asset`
  is the `ar_sprite_slot_frame` frame id.
- **CURSOR bank** = `0x8a765c` = **pool slot 20** → `AR_SPR_TITLE_CURSOR`. The
  menu-selection highlight; frame id = the selected row index.

Both self-decode via the ported `ar_sprite_slot_frame` chain, but stay NULL
until the slot is registered (a real "DATA" resource) AND the **8d** surface
builder (`ar_frame_build_hook`) is wired ⇒ every sprite op no-ops faithfully
(the "still-undecoded" path). So the drive will show a **cleared + flipped
window with no sprites yet** (move B) — proving the loop live and giving 8d a
frame-diff harness.

**Faithful + host-tested arms:** `SURFACE_RESET`→`zdd_object_clear`,
`SURFACE_CLEAR`/`SPRITE`→keyed blit of `frame(main,asset)`→primary,
`SPRITE_LEVEL`→`title_draw_sprite_level` (ramp_b plain+alpha both proven),
`FRAME_END`→`title_compositor_draw` of the bound display-list group,
`FLIP`→present cb, `LOG_FLIPPING`→log cb — the whole intro + menu-background +
fade-out path.

**Deferred behind ctx callbacks** (no-op default): `LOGO`, `SPARKLE`,
`MENU_CURSOR`. These alpha-ramp draws encode a blend-descriptor *pointer* in
`title_draw_cmd.alpha` (a 32-bit field — can't round-trip on a 64-bit host) or
drop the level numerator (`[esp+0x20]`) + fixed src geometry. They only fire
once the run-time ramps (`0x8a92b8`/`0x8a9308`) are populated, never at a cold
boot — so deferring costs no intro/menu-background fidelity. **Wire + validate
these against live goldens during the drive checkpoint** (the command stream
may need enriching for the sparkle src-rect + cursor numerator).

**Fidelity fix:** `TITLE_DRAW_SURFACE_CLEAR` now carries the source frame index
in `asset` (prologue background = 0; logo alpha-0 path = `frames[1]` studio /
`frames[2]` title). The alpha-0 logo blit is `frames[1/2]`, not the
phase-2..3 background `frames[0]` — the op was previously lossy across the two.

**617 host tests (611 pass, 0 fail, 6 skip; +12 this ckpt)**; both 32-bit
cross-builds clean. Ledger **unchanged at 130/1490 (8.1%), 127 tested** — the
sink bridges already-counted functions (no new `FUN_` port tokens).

**Note:** the per-cell DDraw surface builder **8d** (`0x5b9280`,
`ar_frame_build_hook`) is still unported — it's the genuine pixel source. The
sink + drive run *now* with the build hook NULL (blank sprites); 8d fills them
in. See "Render-port task list" below. 8d needs the DDraw god object live, so
it wants **live verification** ([[reference_frida]]).

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
sequencer); port-side `input_trace.{c,h}` + port frame capture (both blocked
on milestone-0 rendering).

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
and **title_sink** (ckpt 21: `title_render_sink` — the cmd-stream→ZDD-blit
bridge resolving banks 19/20, behind `title_render_sink_hook`).
Live boot zero DDERR
through 10 frames in mode 2. **The runner is a tested unit but is NOT yet
driven by `main.c`** — the drop-in still uses its own minimal
`main_loop_body`. The render sink is **ported + host-tested** but **not yet
bound** (no live pixels until the drive wires it + the 8d build hook lands).

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
- **(8d) the per-cell DDraw surface builder `0x5b9280`** — wire
  `ar_frame_build_hook` (+ `ar_frame_free_hook` = `0x5b9390`) to a real keyed
  `zdd_object*` per cell. Pulls in `0x5b6f80` (trim metadata), the format
  switch `0x5b7310/_74f0/_7270` (gated on `[zdd+0x168]` display depth), and
  the 8bpp palette `0x5b7bd0`. Needs the DDraw god object live ⇒ first chip
  that wants **live verification**, not only host tests.
- ~~**(9a) implement `title_render_sink`** over the compositor/wrappers + the
  frame surfaces~~ **DONE ckpt 21** (`src/title_sink.{c,h}` — see the top of
  this file). Resolves banks 19/20, faithful for the intro + menu-bg + fade
  path; LOGO/SPARKLE/MENU_CURSOR deferred behind ctx callbacks.
- **(9b) drive `title_scene_step` from `main.c`** — the next chip (ckpt 22):
  replace the drop-in's `main_loop_body` with a `title_scene_run`, bind the
  sink (`title_sink_bind` with `primary = g_zdd->primary_obj`, a present thunk
  = `zdd_present(g_zdd)`), then live-verify. **(10)** port-side frame capture +
  a `push_comparison.py` (port|retail amplified diff to llm-feed) to close the
  pixel-parity loop against the goldens in `runs/title-idle` & `runs/newgame-full`.

## Next move (pick one — recommendation first)

> Context (ckpt 13): the TAS harness now gives us **retail golden frames** for
> any scripted scene. The natural arc toward "pixel parity on new game" is:
> port the render bridges (move 1) → drive the runner from `main.c` (move 2) →
> capture **port** frames the same way → diff vs the retail goldens captured by
> the harness. Move 1 is still the critical path; the harness is the yardstick.
>
> Two harness-side follow-ups (either self-serviceable or a quick human ask):
> - **Prologue → first-playable-map ground truth** needs a recorded human
>   trace (advance/skip the opening cutscene) distilled to a sparse
>   `{frame,ids}` trace — ask the user to record, or RE the prologue sequencer.
> - **Port-side `input_trace.{c,h}`** (mirror openrecet) is buildable+testable
>   now but latent until `main.c` drives the scene + rendering lands.

> Context (ckpt 21): the **sink is ported** (`title_sink.{c,h}`). What remains
> for "the title scene runs in the real window" is the **drive** — wiring
> `title_scene_step` from `main.c` and binding the sink to the live ZDD. This
> can run *now* with the 8d build hook NULL (= a cleared/flipped window, no
> sprites) — move B — proving the loop live and giving 8d a frame-diff harness.
> Then 8d (`0x5b9280`) fills the sprite surfaces.

1. **(recommended) Drive `title_scene_step` from `main.c` + bind the sink.**
   The sink (ckpt 21) is done and host-tested; this is the step from "tested
   units" to "the title scene runs in the real window". Concretely:
   - **Bind the sink:** after `init_ddraw`, build a `title_sink_ctx` with
     `primary = g_zdd->primary_obj`, `ramp_a`/`ramp_b` = the real `0x8a92b8`/
     `0x8a9308` tables (or NULL for now — they're unfilled at boot anyway),
     `present` = a thunk calling `zdd_present(g_zdd)`, `log_flip` = a
     `log_line("Title Menu - Flipping")`, then `title_sink_bind(&ctx);
     title_render_sink_hook = title_render_sink;`. Also install
     `ar_sprite_decode_hook = ar_sprite_decode` so the banks self-decode once
     registered (still need 8d for real surfaces — NULL build hook = blank
     sprites, which is fine for this move).
   - **The drive:** replace the drop-in's minimal `main_loop_body` with a
     `title_scene_run` that allocates the scene object (owner `sel_list` +
     `input_mgr`), calls `title_scene_init`, then loops `title_scene_step(now=
     GetTickCount(), &hooks)` until `TITLE_SCENE_DONE`, with `title_scene_hooks`
     wired to the real pump / pre/post / per-entry calls (or NULL — they no-op).
     The scene's sprite-group display list (`&scene[esp+0x38]`, the compositor's
     `FRAME_END` arg) is unmodeled — leave `compose_group = NULL` until it's
     traced. Then capture **port** frames the same way the harness captures
     retail and diff vs the goldens in `runs/title-idle` / `runs/newgame-full`.
   - **Wants live verification** (Frida self-serviceable, [[reference_frida]])
     once the window comes up — confirm zero DDERR + a flipping window first,
     then wire 8d for sprites. NB the deferred sink arms (LOGO/SPARKLE/
     MENU_CURSOR) get their real implementations here, validated against the
     live goldens (the command stream may need the sparkle src-rect + cursor
     level-numerator added — see the sink note above).

2. **`mem_watch.py` on the input-ring producer** — the one live probe not yet
   exercised. `mem_watch.py --region <+0x108 addr>:64:input_ring` (no-turbo!)
   to catch the DInput `GetDeviceState` writer (vtable `[0x24]`) that fills the
   ring + the `axis_held` arrays (quirk #41) — the last input black box. Needs
   the runtime address of the input-manager object first (resolve via a hook on
   a known consumer like `0x43c110`). Self-serviceable now ([[reference_frida]]).
   (The call-trace half of the harness is already live-verified — see the
   parity-harness paragraph above; its diff is blocked on Next move #2, not on
   a live run.)

## Open RE threads (see ROADMAP subsystem map for the rest)

- **`FUN_0056aea0`** title scene runner — **fully ported + wired + update
  half complete.** Remaining: implement the draw bridges behind the render
  sink, and drive it from `main.c`.
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
  `ar_sprite_slice` / `ar_sheet_decode_pixels`). **Still unported (the
  genuine pixel source) = the per-cell DDraw surface builder `0x5b9280`**
  (behind `ar_frame_build_hook`; + `0x5b9390` release behind
  `ar_frame_free_hook`, `0x5b6f80` metadata, `0x5b7310/_74f0/_7270` format
  setup, `0x5b7bd0` 8bpp palette) and the logo `0x494e10`. The asset pool
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
