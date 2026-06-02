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

## Method / cross-cutting

- **[cpp-recovery-workflow.md](cpp-recovery-workflow.md)** — `sotes.exe` is a
  32-bit MSVC C++ binary built `/GR-` (no RTTI); how we recover classes,
  vtables, this-call ctors/dtors from the decompile. Read this before
  reverse-engineering any new class.
- **[engine-quirks.md](engine-quirks.md)** — the running catalogue of
  charming / load-bearing / inexplicable engine oddities. **Append to this
  as you find them** (AGENT-WORKFLOW "Note engine quirks"); cite the entry
  number in commits instead of restating the quirk.

## Derived progress artifacts (not findings, but read alongside)

- `../STATUS.md` — coverage headline.
- `../port-ledger.md` / `.json` — per-function port status.
- `../port-frontier.md` — mechanical next-chip list.
- `../ROADMAP.md` — semantic milestone order + binary subsystem map.
