---
name: main-menu-pages
start: end:rh-after-prologue
steps:
  - loaded into a room with control: world != 0 and control
  - still in the game at the end: world != 0 and frame > 4000
forbid:
  - back at the title after loading: world == 0 and tcount == 5 and frame > 6000
---
**Menu probe: every page of the F1 Main Menu** (`docs/game/exercise-checklist.md` §E, `mechanics.md` §11): each top-level
entry; Status for every party member; Equipment (each slot, a costume, an item that "Cannot Equip"); Items (Use on a member,
Discard, Remove / re-arrange), Special Items, Abilities (spellbook assignment Use In Book A/B/C, an AI-use toggle), Party
Commands (Follow Mode, Magic Usage, Formation, Battle Order), Messages, System (every option incl. `Money Display`,
quick-heal config, Options; open Give Up / Return to Title and DECLINE them), `Adventure Stats`. Leave menus with the
same values you found (put back what you changed). Name: menu page / cursor state, the leader's HP/MP, gold.

Start: `rd new <run> --from end:rh-after-prologue` boots with the owner's save after the prologue (Day 5); at the title pick **Continue** and load the newest slot (`tcur`/`tcount` name the title cursor). Look first (`rd look`, F1 → `Messages`) and place yourself in `docs/game/walkthrough.md`. **Name the state you rely on** (`rd ram R mark` → act → `rd ram R diff` → `rd ram R define`) and report each name (address/expression, how validated) — later tasks reuse them. Every screen / outcome you claim: you LOOKED at it (`rd look`, view the PNG). Report per the card, plus guide corrections (what `docs/game/` gets wrong for EN-old) and tool gaps.
