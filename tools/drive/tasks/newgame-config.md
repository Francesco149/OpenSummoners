---
name: newgame-config
start: fresh
steps:
  - a game started: world != 0
  - played into the prologue: world != 0 and frame > 5000
forbid:
  - crashed back to boot: frame > 3000 and world == 0 and tcount != 5
---
**Exercise probe: the New Game config box (docs/game/exercise-checklist.md §A).** From a fresh install (no saves):
title → Start → the config box (`Game Difficulty`, `Auto-guard`, `Start Game`; DRIVER.md: `D D ent` starts with the
defaults).

1. Open the Game Difficulty picker and look at every entry (1:Easy … 4:Expert; is 5:Nightmare listed / locked?) — picture
   of the open picker, the help text of each (`rd text`).
2. Toggle Auto-guard OFF and back ON, then OFF (leave it OFF), pick 3:Hard, Start Game.
3. Confirm the choices took effect in game: name the difficulty and auto-guard state in memory (`rd ram mark` before
   Start → after → `diff`; `diff` = `s32(world+0x2780)` per the vars, 10 = Easy — what is Hard?). Prologue: play until the
   first free control (room / control), then F1 → System → Options → Game & Control: does it show Hard / Auto-guard OFF?
4. Back out WITHOUT saving changes.

Use DRIVER.md menu pacing. Do not read other runs' movies. Stop around 8k frames. Report: run, frames, each screen with its
frame + picture, the difficulty / auto-guard values in memory, game facts [RD], tool gaps.
