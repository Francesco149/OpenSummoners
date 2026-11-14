---
name: town-services
start: end:rh-after-prologue
steps:
  - loaded into a room with control: world != 0 and control
  - still in the game at the end: world != 0 and frame > 4000
forbid:
  - back at the title after loading: world == 0 and tcount == 5 and frame > 6000
---
**Town probe: every service in reach** (`exercise-checklist.md` §F): shops — Buy one item, Sell one (incl. the rare-item
sell confirm and "You have nothing to sell." if reachable) at each shop open now (arms, item / Dad's shop incl. `Help Out`,
boutique); the storage chest (deposit + withdraw); the inn (rest, and the not-enough-money line if you can reach it
cheaply — do not grind); a save statue / fountain; the capsule machine (one try); the sword-lessons NPC (open every
topic); any talkable NPC with a menu. Keep money sane: end with at least what you started with minus what you spent on
the probe. Name: gold, inventory count, the shop / dialogue-menu state.

Start: `rd new <run> --from end:rh-after-prologue` boots with the owner's save after the prologue (Day 5); at the title pick **Continue** and load the newest slot (`tcur`/`tcount` name the title cursor). Look first (`rd look`, F1 → `Messages`) and place yourself in `docs/game/walkthrough.md`. **Name the state you rely on** (`rd ram R mark` → act → `rd ram R diff` → `rd ram R define`) and report each name (address/expression, how validated) — later tasks reuse them. Every screen / outcome you claim: you LOOKED at it (`rd look`, view the PNG). Report per the card, plus guide corrections (what `docs/game/` gets wrong for EN-old) and tool gaps.
