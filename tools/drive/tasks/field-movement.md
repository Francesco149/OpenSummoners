---
name: field-movement
start: end:rh-early-game
steps:
  - loaded into a room with control: world != 0 and control
  - moved around for a while: frame > 6000
forbid:
  - back at the title after loading: world == 0 and tcount == 5 and frame > 6000
---
**Exercise probe: movement & field (docs/game/exercise-checklist.md §B).** Start from the end save of the owner's
`rh-early-game` play (Continue → the newest slot; find out the day / area first: `rd look`, F1 → Messages,
`docs/game/walkthrough.md`). Exercise, each confirmed by state (`lstate`, `lpose`, `lx ly`, `hp`) AND a picture:

1. Walk, double-tap dash (`R . R*20`), slide (↓ while running), crouch, short vs long jump (C held 2 vs 12 frames),
   dash-jump. Report the speeds (lx per frame) and jump heights (ly) you measure.
2. Doors (↑), area exits left / right, stairs.
3. Sword draw / sheathe (Z) and smashing breakables (barrels / pots / crates) for loot; push a crate if one is near.
4. Examine with V vs X on a sign / object; talk to an NPC.
5. Camera: F6 modes; POV keys PgUp / PgDn / Home (`pu pd home`) — what each does to the view (pictures).
6. A field pit if one is reachable (HP cost, where you respawn).

Read `docs/findings/probes/combat-moves.md` / `combat-party.md` for the names already found (lpose, lstate, foe…).
Do not read other runs' movies. Stop around 25k frames. Report: run, frames, each item with its frame, measured numbers,
named state, game facts (corrections to docs/game tagged [RD]), tool gaps.
