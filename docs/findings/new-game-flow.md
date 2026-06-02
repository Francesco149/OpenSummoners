# New-game flow — scene sequence (retail ground truth)

Recovered by the TAS harness (checkpoint 13): a deterministic input trace
injected into retail's input ring, captured frame-by-frame. This is the
reference path the port must reproduce for "structural parity + pixel accuracy
on clicking through new game → first frame in-game".

## The click-through (verified, deterministic)

| stage | how you get there | what's on screen |
|-------|-------------------|------------------|
| **Title menu** | boot, ~frame 1900 menu interactive | Start / Continue / Bonus Menu / Options / Exit. Cursor (►) starts on **Start** (row 0, action `0x1a`). |
| **New-game config** | press **0x24** (confirm) on Start | boxed menu: **Game Difficulty** (1:Easy) / **Auto-guard** (On) / **Start Game**, with a tooltip box. Cursor starts on Game Difficulty. *Separate scene → separate input-manager instance (engine-quirks #43).* |
| **(nav)** | **id 3** (down) ×2 → cursor on **Start Game** | tooltip: "Confirm options and begin the game. (Options can be altered after the game starts.)" |
| **Elemental Stone intro** | press **0x24** on Start Game | a glowing purple gem (the "Secret of the Elemental Stone") on black. |
| **Prologue narration** | (auto-advances) | scrolling story text: "…the country of Scotsholm… Elemental stones are now mass-produced…", gem centered. |
| **first in-game frame** | *(not yet reached — past the prologue)* | TBD — the prologue is a timed/advance-on-input cutscene; best captured from a **recorded human trace** (distil to a sparse trace) or by REing the prologue sequencer. |

## Button ids per menu (injection truth, engine-quirks #42/#43)

- **Title menu** polls `0x22, 2, 4, 1, 3, 0x24`. Effects: id 1 = up, **id 3 =
  down**, id 2 = page-up (no-op), id 4 = page-down (no-op), **id 0x24 =
  confirm**, id 0x22 = abort.
- **Difficulty config** polls `0x22, 1, 3, 0x24, 0x27`. id 1 = up, **id 3 =
  down**, **id 0x24 = confirm/Start Game**, id 0x27 = value left/right (changes
  the Game-Difficulty / Auto-guard option on the focused row).

## The trace

`tests/scenarios/new-game-through/trace.jsonl` (sparse `{frame, ids}`). Frame
numbers are **Flip frames** (same axis the capture uses). They're timing-
sensitive to the menu fade-in gates (`menu_input_sub.ready` must reach 1000 —
the "+0x54 ramp"); the committed trace uses generous margins. Re-time if the
boot path length changes.

## Builder ported (ckpt 37)

The **config-menu grid builder** for the difficulty scene (`FUN_00564780`
case 0x24) is ported in `src/newgame_menu.{c,h}` and verified to emit retail's
exact `TextOutA` stream (quirk #64): a 3×2 linear grid (Game Difficulty /
Auto-guard / Start Game) at box base (32,32), col origins x=72/232, row pitch
28.  Still a **stub in `app_flow`** — the NEW_GAME arm re-enters the title; the
scene is not yet wired as a runnable drive.  Remaining for the live scene: the
run loop (`0x565810`/`0x565d10`), the value toggle (id 0x27, directionally
unverified), the tooltip text node (`0x566850`), the box widget tree
(`0x411940`), and the Start→game transition (`0x564160`→`0x59ec30`).

## Open

- **Prologue → first playable map.** The opening cutscene (stone + narration)
  needs either a recorded human trace (advance/skip presses with real timing)
  distilled to a sparse trace, or RE of the prologue sequencer. This is the
  remaining gap to "first frame in-game rendered".
- The difficulty/auto-guard value toggles (id 0x27) are unverified directionally
  (left vs right vs cycle).
