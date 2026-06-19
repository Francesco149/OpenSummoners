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
