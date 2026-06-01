# Session handoff — last updated 2026-06-02 (skip-splash ported; update half complete, ckpt 12)

**This is the first thing to read at the start of every session.**

Rolling state — REWRITE on each meaningful checkpoint, don't append.
`docs/PROGRESS.md` is the append-only changelog; this file is "where
to pick up *right now*".

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

**Structural-parity harness (offline foundation landed 2026-05-29):**
call-graph diff + mem-watch, mirroring `../openrecet`. How-to:
`docs/parity-harness.md`; design: `docs/plans/parity-harness.md`. Offline
pieces done + tested. The agent + `frida_capture.py` call-trace/mem-watch
modes + `tools/bisect_call_trace_vas.py` are code-complete but **need a
live retail-under-Frida run to verify** (human-verification gate).

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

**`FUN_0056aea0` is fully ported, composed into one runner, AND its update
half is complete (skip-splash landed ckpt 12).** The remaining milestone-0
work is making it *do real work*: implement the draw bridges behind the
render sink so a title frame actually composites, and drive the runner from
the drop-in against the real engine.

## Next move (pick one — recommendation first)

1. **(recommended) Port a draw bridge so the render sink does real work.**
   **Scouted ckpt 12** — the render bridges split into "already ported in
   `zdd.c`" and "the sprite/asset blit subsystem":
   - **Already ported:** `0x5b9410` (surface reset = `zdd_object_clear`),
     `0x5b9b70` (color-keyed blit = `zdd_object_blt_keyed`), `0x5b8fc0`
     (Flip = `zdd_present`). So `TITLE_DRAW_SURFACE_RESET/CLEAR/FLIP` and the
     "plain sprite" wrapper `0x56c610` (a thin forward to `0x5b9b70`) can be
     wired *today* with no new RE.
   - **Still unported (the rabbit hole):** the four title sprite-draw wrappers
     `0x56c470` (cursor), `0x56c4e0` (leveled), `0x56c580` (sparkle), and the
     leveled/sparkle path bottom out in **`0x5bd550`** (302 B blit orchestrator)
     → **`0x5bd680`** (1072 B *software alpha blitter*, pure pixel math, very
     host-testable with synthetic Lock'd buffers) + `0x5b9ae0` (140 B) +
     `0x5b9bf0` (256 B). Plus `0x56c180` (animated sprite-group compositor,
     reads the `DAT_008a760c` sprite pool + `0x4184a0` asset load) and
     `0x418470` (asset get) and `0x494e10` (logo). This is a **multi-checkpoint
     subsystem**; `0x5bd680` is the heart and the cleanest first chip (pure
     RGB565/8bpp blend + colorkey, no DDraw calls of its own — Lock/Unlock are
     its caller's job, already ported). Land it, then `0x5bd550`, then the
     wrappers + a `title_render_sink`.

2. **Drive the runner from `main.c`.** Replace the drop-in's minimal
   `main_loop_body` with a `title_scene_run` that allocates the scene object
   (owner sel_list + input mgr), calls `title_scene_init`, then loops
   `title_scene_step(now=GetTickCount(), &hooks)` until `TITLE_SCENE_DONE`,
   with the hooks wired to the real pump / pre/post / per-entry calls and a
   real `title_render_sink`. Needs at least the Flip bridge (move 1) to be
   visible. This is the step from "tested unit" to "the title scene runs in
   the real window".

3. **Live harness gate** — run `bisect_call_trace_vas.py` /
   `mem_watch.py --region <+0x108>:64:input_ring` under Frida to verify the
   call-trace + mem-watch machinery and catch the input-ring producer (the
   DInput `GetDeviceState` writer, vtable `[0x24]`), which also fills the
   `+0x114/+0x118` axis-held flags. Human-in-the-loop.

## Open RE threads (see ROADMAP subsystem map for the rest)

- **`FUN_0056aea0`** title scene runner — **fully ported + wired + update
  half complete.** Remaining: implement the draw bridges behind the render
  sink, and drive it from `main.c`.
- **Render-half draw bridges** (stubbed behind `title_render_sink_hook`):
  `0x5b9410` (surface reset), `0x5b9b70` (color-key blit/clear), `0x5b8fc0`
  (Flip) are **already ported in `zdd.c`** (wire-up only); `0x56c610` (plain
  sprite) is a thin forward to `0x5b9b70`. The rest are the sprite/asset blit
  subsystem (multi-checkpoint): `0x56c4e0` (leveled), `0x56c580` (sparkle),
  `0x56c470` (cursor) → `0x5bd550` → **`0x5bd680`** (1072 B software alpha
  blitter) + `0x5b9ae0` + `0x5b9bf0`; `0x56c180` (sprite-group compose, reads
  `DAT_008a760c` pool); `0x418470` (asset get); `0x494e10` (logo). See ckpt-12
  scouting in "Next move" #1.
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
