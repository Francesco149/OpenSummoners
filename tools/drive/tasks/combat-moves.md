---
name: combat-moves
start: end:story-day6-barness
steps:
  - loaded into a room with control: world != 0 and control
  - fought for a while: frame > 6000
forbid:
  - back at the title after loading: world == 0 and tcount == 5 and frame > 6000
---
**Exercise probe: combat (docs/game/exercise-checklist.md §C + §E quick-heal).** Start from the Day 6 save (Barness;
`docs/findings/probes/story-day6-barness.md` says where it is). Go to the nearest area with enemies (walkthrough Day 6:
Barness Hills) and, in fights, exercise as much as you can:

1. Every Arche move her level allows (`docs/game/mechanics.md` §2 table: attack chain, jump attack, downstab, combo
   slash, dash attack, …), each at least once on an enemy; note the input that produced it and confirm it by a picture.
2. Manual guard (↑) — and, if the options allow, Auto-guard ON vs OFF (System → Options → Game & Control).
3. A combo of ≥5 hits (combo meter on screen).
4. Switch to Sana (Enter) and cast at least two different spells (hold X, release); one interrupted cast if it happens.
5. F2 quick-heal in combat (charge bar) once; `F` healing key once.
6. Any status effect an enemy inflicts and its recovery message (`rd text`).
7. A level-up if one comes (the Level-up Info screen) — do not grind for it.

Name the state you need (`rd ram R mark` → act → `rd ram R diff` → `define`): leader HP/MP, the enemy HP of the target,
combo counter, the current character. Report each with its address and how you validated it.
Do not read other runs' movies. Stop around 30k frames. Report: run, frames, each move/feature with its frame, named
state, game facts (corrections to docs/game tagged [RD]), tool gaps.
