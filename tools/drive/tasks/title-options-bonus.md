---
name: title-options-bonus
start: fresh
steps:
  - the title menu is up: frame > 600
  - back on the title at the end: world == 0 and tcount == 5 and frame > 3000
forbid:
  - a game started: world != 0
---
**Menu probe: every screen reachable from the title on a fresh install**, without starting a game. From
`docs/game/exercise-checklist.md` §A and §I and `docs/game/mechanics.md` §11: the title (Start / Continue / Bonus Menu /
Options / Exit — cursor starts on Start), **Continue with no save** (what happens?), **Options** — every page:
`Keyboard Setup`, `Gamepad Setup`, `Analog Gamepad Setup`, `Sound Settings`, `Graphics Settings`, `Game & Control
Setup` — open each, move the cursor over every row, change at least one value on each page and see it change, then
leave with `Exit Without Saving` once and with `Save Changes & Exit` once (then put the values back and save again);
**Bonus Menu** — every entry it offers (the guides disagree whether its modes are playable in this edition: find out,
and enter each that opens); `Reset to Defaults`.

The title and option screens are partly sprite-drawn: `rd text` shows the GDI text that is drawn; use `rd look` (Read
the PNG) after each step. Find and name the title's cursor / current-screen state (`rd ram R mark` → move the cursor →
`rd ram R diff`, retail .data is 0x854000..0x8ab2e0) and report it — later tasks will use it. Never pick Exit (it ends
the session) until everything else is done; you may end with Exit as the last action and say so.

Report: run, frames, each screen reached with its frame, the named state (address, meaning, how validated), what the
Bonus Menu really contains, guide corrections, tool gaps.
