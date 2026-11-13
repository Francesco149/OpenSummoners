---
name: gameover-choices
start: end:rh-after-prologue
steps:
  - loaded into a room with control: world != 0 and control
  - still in the game at the end: world != 0 and frame > 4000
forbid:
  - back at the title after loading: world == 0 and frame > 6000
---
**Failure probe: game over, both choices** (`exercise-checklist.md` §H, `mechanics.md` §10): checkpoint, then let the whole
party die in a fight (or a pit / hazard) and see the game-over screen; pick **Try Again** — note exactly where it puts you
and with what HP / money; `rd ck load`, die again, pick **Return To Safety (-10% Money)** — note where, money before /
after. On the way: a party member who falls ("%s has fallen...") and is revived (Revival Drink or an inn / statue — find
out). Name: party HP, gold, the game-over state. Find out which function shows the game-over menu if you can
(the decompile: /mnt/ephemeral/opensummoners/decompiled-ida/enold/all_ida.c; name hints in tools/ida/retail_fields.json).

Start: `rd new <run> --from end:rh-after-prologue` boots with the owner's save after the prologue (Day 5); at the title pick **Continue** and load the newest slot (`tcur`/`tcount` name the title cursor). Look first (`rd look`, F1 → `Messages`) and place yourself in `docs/game/walkthrough.md`. **Name the state you rely on** (`rd ram R mark` → act → `rd ram R diff` → `rd ram R define`) and report each name (address/expression, how validated) — later tasks reuse them. Every screen / outcome you claim: you LOOKED at it (`rd look`, view the PNG). Report per the card, plus guide corrections (what `docs/game/` gets wrong for EN-old) and tool gaps.
