---
name: town-npcs
start: end:rh-water-shrine
steps:
  - loaded into a room with control: world != 0 and control
  - talked for a while: frame > 6000
forbid:
  - back at the title after loading: world == 0 and tcount == 5 and frame > 6000
---
**Exercise probe: talk to every NPC in Tonkiness** (dialogue boxes, portraits, choices, scene scripts — the
text / dialogue renderer and the script engine). Start from the `rh-water-shrine` end save (Continue → newest slot: [RD town-npcs] it is Day 6 — Sana has a fever, Arche alone).

1. Visit every Tonkiness room you can enter without leaving town (house 210140 / 210141, street 210120, town 210110,
   inn 210150, shops, the boutique, the school area if open) and talk (X, facing them) to EVERY person and animal; also
   examine (V) signs and notable objects. Advance text with `ent` (a press during typing completes the line).
2. For each: room, lx, who, the first line (`rd text`) and whether a choice / portrait / item came up. Picture of any box
   with a portrait or a choice.
3. Talk to the same NPC twice where the line might change; (Sana is not in the party on Day 6.)
4. Do NOT buy anything (shop Buy lists buy on `ent`; leave with `C`). Do not accept quests that leave town.

Use DRIVER.md (exit zones, X vs V). Do not read other runs' movies. Stop around 25k frames. Report: run, frames, a table
room / lx / NPC / first line / frame, scenes triggered, game facts [RD], tool gaps.
