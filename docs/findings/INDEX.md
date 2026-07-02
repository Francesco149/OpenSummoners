# Findings — subsystem RE writeup index

Map of the durable reverse-engineering writeups under `docs/findings/`.
Each captures the **why** behind our port choices: which Ghidra addresses
cover which behaviour, struct offsets, calling conventions, retail quirks.
Cite the `by-address/<va>.c` form (addresses are stable; names rename).

> New to the project? Read order: `STATUS.md` → `memories/HANDOFF.md` →
> this index → the specific subsystem doc you're about to touch.

## Boot path (entry → first frame)

- **[winmain-and-bootstrap.md](winmain-and-bootstrap.md)** — PE entry →
  window class → message pump → frame limiter. The spine everything else
  hangs off.
- **[launcher-dialog.md](launcher-dialog.md)** — the startup launcher
  DLGPROC (mode select: Full/Safe/Wind/DB/Zoom). Currently stubbed by the
  `--launcher-mode=N` CLI flag; the real `config.dat` parse is FUN_005a4770.
- **[ddraw-init.md](ddraw-init.md)** — DirectDraw 7 bring-up:
  `DirectDrawCreateEx` → cooperative level → CreateScreen 5-mode dispatch →
  clipper/palette attach. Mostly ported (ZDD wrapper).
- **[audio-init.md](audio-init.md)** — DSound + DInput init from the
  post-launch driver. (Audio playback paths still unported.)

## Editions & versions

- **[game-editions-and-voice.md](game-editions-and-voice.md)** — the JP-SE / EN-SE /
  EN-old build matrix: file inventory, shared-vs-different asset DLLs (with hashes), the
  voice-line architecture (`sotesx_s.dll` = 1,448 `WAVE` clips, **JP-only**), which engine
  loads which DLL, and why the English builds are silent. Grows as we compare more.

## Rendering / assets

- **[palette-session.md](palette-session.md)** — palette-ramp leaf helpers +
  the deferred PE-resource bitmap decoder.
- **[asset-loader.md](asset-loader.md)** — how the engine pulls asset bytes
  from `sotesd.dll` / `sotesw.dll` / `sotesp.dll` + PE resources. The big
  data archives.
- **[sprite-pipeline.md](sprite-pipeline.md)** — asset pool `DAT_008a760c` →
  sprite bank (`ar_sprite_slot`) → frame getter (FUN_00418470, ported) →
  compositor (FUN_0056c180, decoded) → `zdd_blit_orchestrate`. The chain
  behind the render sink's sprite draws. The sprite-sheet decoder
  (FUN_004184a0) is the next chip.
- **[0057ca40-rabbit-hole.md](0057ca40-rabbit-hole.md)** — FUN_0057ca40, the
  group-3 sprite batch that turned out to span multiple subsystems. A
  cautionary tale + partial port.

## Scene / gameplay

- **[title-scene.md](title-scene.md)** — the title-menu scene runner
  FUN_0056aea0 (3441 B): phase breakdown, the PTR_DAT_0056bfa4 jumptable,
  the helpers it needs. **This is the active forward-path target.**
- **[menu-list.md](menu-list.md)** — the menu-list controller: scroll-into-
  view (FUN_004192b0), the cursor-nav engine (FUN_0043ca40, jump table
  recovered) and the input-action latch (FUN_0043ce50). Completes the
  poll → latch → nav input chain. **Ported, checkpoint 4.**
- **[butterfly-direction-sprite.md](butterfly-direction-sprite.md)** — the town
  butterflies' 8-way flight render (THEME 2 notes #0+#2). The frame model
  `cel = d*4 + flap` (d = an 8-way wander heading, not the velocity) is PROVEN off
  `retail.osr`; the port renders only 2 facings. The render path + the one open
  link (which field carries `d`) for the `butterfly-wander`/`-flutter` debts.
- **[npc-colour-variant.md](npc-colour-variant.md)** — the townswoman's palette swap
  (THEME 2 note #1). The girl = map townswoman `0xc440`, colour variant `param_11` =
  map record `+0x2c` = 1; the variant bank is a CLONE of the base 8bpp sheet + a
  palette-INDEX-remap (`DAT_006748d0`). The clone + remap pointer are already in the
  port; what's missing is the deferred render consumer that APPLIES the remap, for
  the `effect-color-variant` debt.
- **[arche-house-turn.md](arche-house-turn.md)** — Arche's house listen→face-father
  turn (USER studio notes #3-5). The script emote `0x401e60(Arche,1)` at
  `0x4d7d80:1170` (after house L5) = actor command kind 2 ("turn to face dir 1",
  `0x43e5b0` case 2); cels `158(4t)→7(4t)→` the base-0 standing idle, RE'd off
  `retail.osr` res `0x570`. **Ported drawcall-faithful (ckpt 146)** as a fire-and-
  forget `CS_ACT_ACTOR_TURN`; the ~7t absolute-tick lag is the house-cadence phase debt.
- **[errands-render-gaps.md](errands-render-gaps.md)** — the remaining errands/freeroam
  gaps from the USER's `osr_notes.jsonl`, each RE'd off `retail.osr`: the shelf/bookshelf
  props (**FIXED ckpt 146** — a Z-ORDER bug: the ckpt-145 ERRANDS_CAST furniture at cast
  layer 13 occluded the layer-8 structure props; background furniture → layer 7), the
  fireplace FIRE (`res=1034`, alpha `bmode=1` `st=0x8000`, frames 0-5 @329,178, port draws
  none — bank not loaded; deferred), the freeroam HUD (`res=0`: HP/MP/level/★★ panel), and
  the wall tint. The exact USER note crops + ticks + the `osr_prof` recon recipe. LESSON:
  a "missing" element may be emitted-but-occluded — check the draw-stream seq (z), not just
  "is it drawn".
- **[dialogue-body-row-distribution.md](dialogue-body-row-distribution.md)** — the
  line-count-dependent vertical spacing of the dialogue body text (USER: a 3-line line
  shows only 2 lines + too-tall spacing).  `FUN_0048da70` distributes the rows in a fixed
  3-row grid: `gap = min(20, ((3-rows)*28)/(rows+1))`, row Y = `box_y + 20 + (r+1)*gap +
  28*r` — so 1 row→gap 20, 2 rows→9 (pitch 37), 3 rows→0 (pitch 28).  Constants RE'd
  (`base_y=20`/`max_rows=3` = `FUN_0040df40` params; `max_gap=20` = `FUN_00410610:19`;
  `pitch=28` by formula consistency), retail.osr-verified bit-exact at every arrival line.
  **Ported (ckpt 149)** — `dialogue_body_row_dy()` replaces the old constant pitch.
- **[errands-opening-dialogue.md](errands-opening-dialogue.md)** — the errands-scene opening dialogue
  (USER ckpt 152, the movement tutorial).  The questline `0x4dc510`'s entry case plays 3 Arche lines via
  `FUN_0049d6e0` — RE-confirmed to be the SAME dialogue display the town chain drives, so it is 3 more
  lines through the existing box system, played concurrent with freeroam control.  Lines/faces/box
  ground truth + the port (a 1-room cutscene chain re-armed after the entry reveal) + tick-aligned
  verification.  **Ported ckpt 152**; residual = the line-3 inline button icons (`PORT-DEBT(dialogue-arrow-art)`).
- **[dialogue-advance-early.md](dialogue-advance-early.md)** — the "port skips dialogue early" divergence
  (USER studio mark t1197, ckpt 176).  DISPROVES the advance-gate hypothesis: the advance PRESSES already
  match retail (port DLGT + dialogue_timeline off port-stairs|retail-stairs).  The mark = a box RENDER
  linger (arrival L9 box cleared 8t early); **fixed** `ARRIVAL_EXIT_BOX_HOLD=8` (== retail 1200).  OPEN:
  the residual house/errands EARLY-start is a separate beat-DURATION gap (−10t/−8t at the room fades).
- **[freeroam-collision.md](freeroam-collision.md)** — the 442a70 tick GEOMETRY (ckpt 175):
  the support probe (a delta=+1 `0x54e5c0` call → `body+0x24`), the vertical mover + ledge
  walk-off → FALL, the horizontal mover `0x54db10`/`0x54ded0` (step-up stair climb + step-down
  floor hug, raw-disasm arg truth — Ghidra numbering wrong on both movers), `body+0x20` = the
  drop-through timer, the slope-profile ramps read live off the user's exe, and the
  `LAB_00589520` "occlusion marks" that are really INVISIBLE COLLISION WALLS (the errands left
  wall).  Retired `char-collision-mover`/`collision-slopes`/`decode-occlusion-mark`; the
  stairs-sweep wx verify is bit-exact through the climb both sides.
- **[freeroam-hud.md](freeroam-hud.md)** — the `res=0` freeroam status HUD (USER notes #7-9,
  errands tick 2413).  The full drawcall ground truth (the seq 462-536 overlay layer: top-left
  leader panel = portrait + HP/MP bars + numbers + level + stars; bottom strips; the 6-slot item
  bar; the door indicator) + the render architecture (`FUN_00494e60` orchestrator, ×2 from the
  render driver `0x48c150`, + its ~15 sub-renderers) + the dependencies (res=0 UI source sheets
  [recoverable from the `.osr`], the unported party context → a captured `PORT-DEBT(hud-party-context)`
  stand-in) + the incremental port plan.  `tools/trace_studio2/hud_probe.py` dumps the HUD layer at
  any tick.  **Scoped ckpt 152; port in progress.**
- **[dash-double-tap-trigger.md](dash-double-tap-trigger.md)** — the freeroam DASH
  trigger (`char-run-trigger` RETIRED, ckpt 150).  The run PHYSICS was bit-exact (ckpt
  118); the run FLAG now derives from the live event ring: `input_dash_double_tap`
  (`0x479e70`/`0x479960` reduced — two same-dir presses within the config window
  `*(*0x8a6e80+0xf8)` = **800 ms**, read live) + `character_resolve_run` (the
  dash-resolution half of the char-AI `0x478ba0`, with retail's self-sustain), fed into
  `character_step` by `freeroam_step`.  Host-verified end-to-end (a double-tap → RUN cap
  48000; a single press → WALK cap 24000).  Quirk #113.
- **[freeroam-pose-commands.md](freeroam-pose-commands.md)** — the freeroam U/D-POSE
  commands (CROUCH / SLIDE / UP-defensive, ckpt 153).  The COMMAND layer is ported:
  `character_resolve_pose` (the `cmd[3]` half of `0x478ba0:248-259` — DOWN→10 / UP→0xb off
  a held axis + a ring [10,800]ms find + self-sustain) + `input_ring_find_recent`
  (`0x479960` w/ NULL used-map) + the **ring-id fix** (input.h had DOWN/UP backwards;
  UP=1/DOWN=3).  The APPLY physics (apply states 2/5/6 — accel-disable for the crouch/
  up-stop, distinct accel/cap for the slide) is RE'd structurally but needs a live const
  capture (`char-pose-physics`).  6 host tests; binary-verify via the `fr_pose` OSR_STATE.

## Method / cross-cutting

- **[cpp-recovery-workflow.md](cpp-recovery-workflow.md)** — `sotes.exe` is a
  32-bit MSVC C++ binary built `/GR-` (no RTTI); how we recover classes,
  vtables, this-call ctors/dtors from the decompile. Read this before
  reverse-engineering any new class.
- **[engine-quirks.md](engine-quirks.md)** — the running catalogue of
  charming / load-bearing / inexplicable engine oddities. **Append to this
  as you find them** (the format + retail-only rule are in that file's header,
  per CLAUDE.md); cite the entry number in commits instead of restating the quirk.

## Harness / tooling

- **[ddraw-blit-trace.md](ddraw-blit-trace.md)** — the DDraw blit-command +
  state trace (Phase-B B3): the `render_id` cross-side identity
  (`(resource_id, frame)` + a decoded-sheet `dhash` fingerprint), per-blit
  emission at the 5 primitives, the retail Frida mirror, and `render_diff.py`
  (names the wrong DRAW; `flow_diff` names the wrong LOGIC). Live-verified.
- **[tas-harness.md](tas-harness.md)** — the 100%-deterministic port↔retail
  trace-diff stack: the `--lockstep` retail clock (1 update/present, matching
  the port), bilateral TAS anchors (subtitle_anim_start / newgame_enter /
  prologue_enter, RNG-stamped), and `tools/tas_diff.py`. Intro bit-exact
  (28/28); prologue cutscene content bit-exact (63/64) with two surfaced
  threads (port skips the new-game→prologue transition; RNG desync there).

## Derived progress artifacts (not findings, but read alongside)

- `../STATUS.md` — coverage headline.
- `../port-ledger.md` / `.json` — per-function port status.
- `../port-frontier.md` — mechanical next-chip list.
- `../ROADMAP.md` — semantic milestone order + binary subsystem map.
