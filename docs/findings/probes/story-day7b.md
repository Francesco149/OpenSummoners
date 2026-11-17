# probe story-day7b

run `story-d7b`; accepted 2026-11-17 14:23

check PASS (tools/drive/tasks/story-day7b.md)
verify MATCH (+ real device)
filed tests/scenarios/story-day7b/trace.jsonl (339205 frames)

## driver's report

## story-d7b: Lead Kobold beaten, Loch Carudale crossed, now inside the Carudale Tunnels (330120). Stopped because the budget ran out.

**Status:** the movie ends at **f339205**, about 24.5k frames past the start (f314753). `rd check` **PASS** (step 1 at f779, step 2 at f3001, the forbid never fired). `rd verify` **MATCH** twice: at f334714, about the $6 mark, and again at the final f339205.

The run stands in **330120** at lx 66992, ly 61600, inside the ↑ zone 3 (→ 330125). Arche has 121/188 HP and Sana 77/104. Sword drawn, 2325 CP, Arche leading. The door will not fire yet because Lead Merkid-class enemies with 53–124 HP are within range (`zrest` stays at 30). They must be killed first.

### Route reached (frames)
- **f314850:** the leftover 260180 fight is over (search with `fh == 0`) and the party is in 260190.
- **f315423:** read the 260190 sign. I first went **left (zone 3) to 265170 → 265160**. That is the **Chartreux road**: it is blocked at lx ≈ 214800 by a wall of burnable vines (objects 70400; the sword does not cut them). The 265160 chest (225600) gave **300 CP at f318177**. Then I went back.
- **f319491:** 260190 gate (zone 2, ↑ at x 44800) → **260200**. This is the crate puzzle; see the corrections below.
- **f321015:** → **260210**. Its zone 3 (35200, ↑) leads to 410110, the Tower. I went on left.
- **f321542:** → **260215**. The kobold scene starts on entry. The boss fight began at **f323094**.
- **f325334: Lead Kobold defeated.** Snivelly fell first, at f323705. Sana went to Lv5. The rescue talk with the couple ran f325634–327659 (the couple run a Chartreux bakery; the "Dark Witch" rumour).
- **f327809:** → **260220**. Exit zone 2 at (0, 12800) is held by Mothbees and a Merkid Mage; they were cleared at **f330457** → **260230, Loch Carudale**.
- **Loch:** talked to the old man on the pier at f331560 ("the loch is quite deep"). With Sana as leader I swam to the underwater ledge chest (150400, 51200), but **could not open it**: V / X / U did nothing while swimming, and two Merkids kept hitting her. Then I swam to the far-left shore. Sana's Heal Water (`n2`, hold X 140 frames) took her 15 → 104 HP. ↑ at the cave (zone 2, x 105600) → **330110, Carudale Tunnels**, at **f334189**. The whole party moved, Arche included, even though she had been left on the pier. Intro talk until f334714.
- **f337354:** → **330120**. Sana had been KO'd (0 HP) on the way. I used the **Revival Drink** on her (→ 104) and the **Mystery Juice** (it was in the inventory) on Sana.

### Saves
No new save: there was no statue, inn or fountain on this stretch, so `rd endsave` was not run. The last save is still the Tonkiness inn, `d89ccc7f7e38`.

### Party at the end
- **Arche:** Lv6 (adventurer level 3 + combat level 3, cap "MAX: 8" = 3 + 5), Marks 5/20, 121/188 HP. Iron Sword and Leather Armor; Armor #2 and Accessory empty.
- **Sana:** **Lv5**, 104 HP, MP max raised by the Mystery Juice. Iron Staff and Cloth Armor.
- **Items:** about 3 Herbs, Magic Candy, Guardian Drop, Underwear. The Revival Drink is used up.

### Upgrades taken and skipped
- **Cloth Armor on Sana:** it was **already equipped** at the start (the Status page shows Defense 23/18), so the card's note was out of date.
- **Arche's cap:** "MAX: 8" is right. The level shown is adventurer level + combat level, and combat level is capped at 5 with 5 Marks. I did not reach MoH 7, so I could not check the cap after it.
- **Taken:** healed before and during the boss with F2 (out of combat the Herb is instant; in combat it is a cast bar of about 250 frames). Revival Drink on downed Sana. Mystery Juice → Sana.
- **Skipped:**
  - Mermaid Scale: the chest would not open while swimming.
  - Canned Salmon dock pot (70304 at 270600): V and X did nothing.
  - The old man by the boat on the left shore (about lx 111000): pressing ↑ near him entered the cave instead.
  - MoH 7, Cloth Gloves, Chartreux: not reached.

### Named state (defined in this run)
- `lk` / `sk`: Lead Kobold / Snivelly HP. They are char slots 14 / 15 (`s32(s32(s32(world+0x1198)+0x750)+0x54)` and `+0x119c`), max 430 / 77, codes 100171 / 51030. `lkx` / `skx` are their x.
- `ahp` / `shp`: HP of party slots 0 / 1. `c0`..`c3`, `h0`..`h3`, `fh`, `tx`, `ty`: per-slot hostile HP and position.
- `crx`: x of the 260200 crate (zone actor 0, body +4).

### Walkthrough corrections and game facts
- **The 260190 sign's ↑ (the gate, zone 2) is the Weathervane road. ← (zone 3, 265xxx) is the Chartreux road**, and it is closed by burnable vines in 265160.
- **260200 crate (70205, starts at x 19200 in the alcove under the ledge):**
  - A run-jump left gets you *behind* it (search `LC*8 L . L*9 … LC*51`).
  - Push it right (sword sheathed) to x 43000, out from under the overhang, then climb up to the exit at (0, 12800).
  - A full jump rises about 14200 in `ly`, not 8400.
- The 260215 boss scene fires on entering the room. There is no chance to heal before the fight, but F2 works during it.
- Loch Carudale: Arche cannot cross the lake, but entering the cave with Sana takes Arche along.
- Merkids underwater hit Sana hard (104 → 15).
- **Swapping leaders:** `ent` needs a fresh edge. A play that stops on an `ent` frame swallows the next `ent`. Tapping `ent` with `--until "control and frame>N"` swaps the leader again and again.
- **Menus:** `esc` does not close the F1 menu; `bk` does.
- **Talking:** Sana's X attacks NPCs; Arche's X with the sword sheathed talks.

### Tool gaps
- Fight searches ran at 85–290 ticks/s.
- Underwater chests could not be opened with the input set I tried.
- The 598-token limit forced shorter `tx` / `fh` expressions.
- `rd tap --text` misses lines while they are still being typed.
- A search that reports `found: false` without `--apply-best` throws away its best path. Rerunning it is random, so the path is lost.

---
model claude-opus-5-5, effort high, 224 turns, $7.29, 122 min

