---
name: party-magic-controls
start: end:rh-after-prologue
steps:
  - loaded into a room with control: world != 0 and control
  - still in the game at the end: world != 0 and frame > 4000
forbid:
  - back at the title after loading: world == 0 and tcount == 5 and frame > 6000
---
**Party / magic probe** (`exercise-checklist.md` §D, `mechanics.md` §1, §4, §5): switch character (Enter) through every
member; cast at least one spell with each caster (select with Z / numpad, hold X, release) — once interrupted, once
completed; F3 cycle Follow Mode (all three), F4 cycle Magic Usage (all six), F5 spellbook A/B/C, F6 camera modes, F2
quick-heal (out of combat; in combat if a fight is near); guard with ↑ (Auto-guard off, then back on via Options);
double-tap dash; crouch / slide. In a fight with weak enemies, land a combo of 5+ hits (combo meter). Name: the active
character, each member's HP/MP, follow mode, magic usage, spellbook, camera mode.

Start: `rd new <run> --from end:rh-after-prologue` boots with the owner's save after the prologue (Day 5); at the title pick **Continue** and load the newest slot (`tcur`/`tcount` name the title cursor). Look first (`rd look`, F1 → `Messages`) and place yourself in `docs/game/walkthrough.md`. **Name the state you rely on** (`rd ram R mark` → act → `rd ram R diff` → `rd ram R define`) and report each name (address/expression, how validated) — later tasks reuse them. Every screen / outcome you claim: you LOOKED at it (`rd look`, view the PNG). Report per the card, plus guide corrections (what `docs/game/` gets wrong for EN-old) and tool gaps.
