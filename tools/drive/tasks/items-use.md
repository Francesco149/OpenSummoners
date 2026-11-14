---
name: items-use
start: end:story-day6-barness
steps:
  - loaded into a room with control: world != 0 and control
  - used items for a while: frame > 4000
forbid:
  - back at the title after loading: world == 0 and tcount == 5 and frame > 6000
---
**Exercise probe: using items (docs/game/exercise-checklist.md §E).** Start from the Day 6 save (Continue → newest
slot: Barness inn, Arche alone). The combat-moves probe saw permanent stat items in the bag (Power Drop, Good Child Drop,
Guardian Drop, Mystery Juice) and no healing items. Read `docs/findings/probes/main-menu-pages.md` (the Items page
layout) first.

1. For EACH permanent stat item: F1 → Status (note every stat) → Items → Use → on Arche → the message → Status again. Report
   which stat changed by how much (pictures of both Status pages), and find the stat's address (`rd ram mark` → use → `diff`
   within the leader's stat block `s32(s32(lslot+0x9f4)+0x750)`).
2. Buy a healing item if a shop is near (the inn's item shop: a Barness kid says it is in the inn) and use it from Items at
   full HP (what message?) and after taking damage (HP change).
3. Items that "cannot be used" here (equipment, Special Items): the message.
4. Discard something cheap (confirm prompt) — not a rare item; then the Z re-sort.
5. If a Recall Lamp or any travel item is in the bag: use it (and where it is blocked).

Do not read other runs' movies. Stop around 15k frames. Report: run, frames, each item with its frame + message + stat
change, named state (stat addresses with how you validated them), game facts [RD], tool gaps.
