# Session handoff — last updated 2026-06-02 (sprite frame getter ported, ckpt 16)

**This is the first thing to read at the start of every session.**

Rolling state — REWRITE on each meaningful checkpoint, don't append.
`docs/PROGRESS.md` is the append-only changelog; this file is "where
to pick up *right now*".

## ⭐ NEW (ckpt 16): the render sink is gated on an ASSET/SPRITE subsystem — first chip (frame getter) ported

Scouted move #1 ("build the render sink + drive the runner") and found it is
**not just wiring** — every sprite draw resolves a *frame surface* out of an
unported asset/sprite pipeline. Fully decoded the chain and documented it in
**`docs/findings/sprite-pipeline.md`** (read this before the next chip):

```
asset pool DAT_008a760c[bank_id] → ar_sprite_slot (the "bank", already modeled!)
  bank->entries[0].frames[frame] → zdd_object* (the frame surface)
     ↑ lazily decoded by FUN_004184a0 (sprite-sheet loader) on first use
compositor 0x56c180 walks the scene's display list → zdd_blit_orchestrate per entry
sprite wrappers 0x56c610/_4e0/_470/_580 each resolve ONE sprite the same way
```

**Key reuse find:** the sprite "bank" is the **already-pinned `ar_sprite_slot`**
(0x44 B, `asset_register.h`) indexed via the existing `ar_pool_get_slot` — I
started to duplicate it as a fresh `zdd_sprite_bank` and reverted. Build on
`ar_sprite_slot`, not a parallel model.

**Ported this ckpt:** the frame getter **`FUN_00418470` → `ar_sprite_slot_frame`**
in `asset_register.c` (the two-level `slot->entries[0].frames[id]` lookup with
lazy decode behind the nullable `ar_sprite_decode_hook`). Widened
`ar_sprite_entry.a` (was opaque `uint32_t`) → **`void *frames`** to pin its role
+ make the getter host-testable (still 4 B on 32-bit; 8-byte record holds).

**562 host tests pass, 0 fail, 6 skip** (4 new getter tests). Both 32-bit
cross-builds clean. Ledger **126/1490 touched, 123 tested** (+1 getter).

**Next move = the compositor `0x56c180`, then the decoder `0x4184a0`.** The
compositor is **fully decoded in sprite-pipeline.md** (frame-index + centi-pixel
geometry + the `0x8a92b8` blend-desc ramp clamp); it needs a NEW render-bridge
module that includes both `asset_register.h` (slot/getter) and `zdd.h` (blit +
primary surface) — the home for the `0x56cxxx` wrappers too. The decoder
`0x4184a0` (24bpp decode + per-channel brightness LUT + `0xff00ff` key + frame
slice via `0x4188b0`) is the genuine pixel source — nothing renders without it,
and it needs the sprite-sheet binary format pinned first. THEN the render sink +
drive from `main.c`. See "Render-port task list" below.

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

## Module inventory (11 modules)

Pixel-Drawer, Asset-Register, Bitmap-Session, WndProc, ZDD wrapper,
cs_dispatch, app_pump, title_scene (`FUN_0056aea0` — **fully ported and
wired**: fade FSM + pacing FSM + update half + render half + the
**`title_scene_step` orchestrator** = the whole outer loop), input
(`FUN_0043c110`), obj_container (`FUN_00412c10` + `FUN_00414080`), menu_list
(`FUN_004192b0` + `FUN_0043ca40` + `FUN_0043ce50` + `FUN_0040f5c0` +
`FUN_0040e0c0` + `FUN_00411f40` + `FUN_0040f3e0`). Live boot zero DDERR
through 10 frames in mode 2. **The runner is a tested unit but is NOT yet
driven by `main.c`** — the drop-in still uses its own minimal
`main_loop_body`, and the render bridges behind `title_render_sink_hook` are
still stubs (no pixels yet).

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
~~(8a) sprite frame getter `0x418470`~~ **DONE ckpt 16** (`ar_sprite_slot_frame`
in `asset_register.c`; the `slot->entries[0].frames[id]` lookup + lazy-decode
hook). **The remaining render-side chips, in dependency order (all decoded in
`docs/findings/sprite-pipeline.md`):**
- **(8b) the compositor `0x56c180`** + the wrappers `0x56c470`/`_4e0`/`_580`
  (and the trivial `0x56c610`). Fully decoded. Needs a NEW render-bridge module
  (`title_render.c`/`.h` say) that includes both `asset_register.h` and `zdd.h`.
  The compositor walks the scene sprite-group display list (entry layout +
  geometry + `0x8a92b8` ramp clamp all in the findings doc) → `zdd_blit_orchestrate`
  per entry onto `0x8a93cc->[0x16c]`.
- **(8c) the sprite-sheet decoder `0x4184a0` + slicer `0x4188b0`** — the
  `ar_sprite_decode_hook` target; the genuine pixel source (24bpp decode +
  per-channel brightness LUT + `0xff00ff` key + frame slice). Pin the
  sheet binary format first.
- **(9) drive `title_scene_step` from `main.c`** + implement `title_render_sink`
  over (8b)+(8c). **(10)** port-side frame capture + a `push_comparison.py`
  (port|retail amplified diff to llm-feed) to close the pixel-parity loop
  against the goldens in `runs/title-idle` & `runs/newgame-full`.

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

1. **(recommended) Build `title_render_sink` + drive the runner from `main.c`
   (the old move #2).** The blit-primitive layer is complete (ckpt 15), so
   there is **no more pure logic to port first** — the path to milestone-0
   pixels is now wiring. Two intertwined pieces, do them together:
   - **The sink:** implement `title_render_sink` (behind `title_render_sink_hook`)
     to turn each `title_draw_cmd` into real zdd calls:
     `TITLE_DRAW_SURFACE_RESET`→`zdd_object_clear` (`0x5b9410`),
     `..._CLEAR`/`..._SPRITE`→`zdd_object_blt_keyed` (`0x5b9b70`),
     `..._SPRITE_LEVEL`→ either `0x5b9b70` (ramp 0) or `zdd_blit_orchestrate`
     (`0x5bd550`), `..._MENU_CURSOR`→`zdd_blit_orchestrate`,
     `..._FRAME_END`→ the compositor loop (`0x56c180`: walk the sprite-group
     display list, `zdd_blit_orchestrate` per entry), `..._FLIP`→`zdd_present`
     (`0x5b8fc0`). The new RE this needs is the **engine state the draws read**:
     the asset/sprite pool `DAT_008a760c` (+ lazy load `0x4184a0`/`0x418470`),
     the frame-lookup math in `0x56c180`, the god object `0x8a93cc->[0x16c]`
     (primary surface), the two alpha ramps `0x8a9308`/`0x8a92b8` (quirk #40/#45),
     and the logo path `0x494e10`. Model these as they're touched.
   - **The drive:** replace the drop-in's minimal `main_loop_body` with a
     `title_scene_run` that allocates the scene object (owner sel_list + input
     mgr), calls `title_scene_init`, then loops `title_scene_step(now=
     GetTickCount(), &hooks)` until `TITLE_SCENE_DONE`, hooks wired to the real
     pump / pre/post / per-entry calls + the real sink above. Then capture
     **port** frames the same way the harness captures retail, and diff against
     the goldens in `runs/title-idle` / `runs/newgame-full`. This is the step
     from "tested unit" to "the title scene runs in the real window".
   - NB this likely wants **live verification** (Frida self-serviceable,
     [[reference_frida]]) once pixels appear, not just host tests.

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
- **Render-half draw bridges** (stubbed behind `title_render_sink_hook`).
  **The whole blit-primitive layer is now ported** in `zdd.c`: `0x5b9410`
  (surface reset = `zdd_object_clear`), `0x5b9b70` (color-key blit/clear =
  `zdd_object_blt_keyed`), `0x5b8fc0` (Flip = `zdd_present`), `0x5bd680`
  (alpha core, ckpt 14), `0x5bd550` (orchestrator = `zdd_blit_orchestrate`,
  ckpt 15), `0x5b9ae0` (`zdd_object_blt_rects`, ckpt 15, dead-path only).
  The sprite frame getter `0x418470` is now ported (`ar_sprite_slot_frame`,
  ckpt 16). **Still unported = the WRAPPERS + the compositor + the decoder**,
  all fully decoded in **`docs/findings/sprite-pipeline.md`**: `0x56c610`
  (plain) / `0x56c4e0` (leveled) thin forwards; `0x56c470` (cursor) / `0x56c580`
  (sparkle, also `0x5b9bf0` 256 B) / the compositor `0x56c180`; the sheet
  decoder `0x4184a0`/`0x4188b0` (the `ar_sprite_decode_hook` target — the real
  pixel source); and the logo `0x494e10`. The asset pool `DAT_008a760c` is the
  already-modeled `ar_sprite_slot` pool (`ar_pool_get_slot`).
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
