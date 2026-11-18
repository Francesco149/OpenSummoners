# probe story-day7d

run `story-d7d`; accepted 2026-11-18 00:20

check PASS (tools/drive/tasks/story-day7d.md)
verify MATCH (+ real device)
filed tests/scenarios/story-day7d/trace.jsonl (39893 frames)

## driver's report

# story-d7d: Chartreux City, gear, Sweet Sticks (Day 7)

**Run:** `story-d7d`, from `save:1e239cb938ff` (slot 12, Dancing Piglet Inn). It ends at **f39893**, 39.0k frames after the load at f862. All on RD_HOST=bunker. Nothing touched Windows.

**Gates:** `rd check story-d7d tools/drive/tasks/story-day7d.md` **PASS**. `RD_HOST=bunker rd verify story-d7d` **MATCH** (f39893, rng 18bcffe9, semv dca66a55d8e5da73, fb 519b40d0438aa513).

**End save:** Dancing Piglet Inn (300240), slot **13** (page 2), saved at f39750. `rd endsave` → **`5e899fce8699`**. The next leg starts with `rd new NEXT --from save:5e899fce8699`.
- On the load screen, pick slot **13** by hand: its in-game date (12:27) is older than slot 12's (13:58).
- Load path: title cursor `tcur` = 2 at boot, so press `U`, then Continue. The load list opens on page 1 (slot 7); press `R` for page 2.

**End state:** f39893, room 300240, inn lobby 2F, control. Gold 1003 CP. `story` = 1210 (no story-flag change through the sweet-stick quest). Both members at full HP.

**Not reached: step 6, the trip home to Tonkiness.** The ~40k-frame budget ran out at the Chartreux inn. The next leg starts at the inn, already holding [Taiyaki Sweet Sticks].

## Route steps (frames)
- f862: loaded with control in 300240.
- f1116: **Mystery Juice → Sana** (Max MP +4). f1438: **Guardian Drop → Arche** (Resist +2).
- f3796: inn 3F right room (300245, closet 300245603 at (66800, 7700)) → **[Chest Sash]**.
- f6360: weapons store 300220 (in 300110, door x 220800).
  - f7716: bought **Thick Leather** (950) for Arche. Her old Leather Armor → Sana (f8657).
  - f9398: drawer 300220601 upstairs (78400, 12800) → **[Power Drop] → Arche** (ATK +1, f9695).
  - **The 30 CP barrel (70302, handle 300220501, (70600, 52800)) gave nothing**: a 90 s search with V / X never raised `gold`. It was probably looted already, or the item is somewhere else.
- f13491: 300280 (first building past the stone archway, 300130 x 297600), 2F closet 300280601 → **[Woven Gloves]**. Equipped on Arche (DEF +2); the Chest Sash went to Sana (0 stats; "wards off cold").
- f15224–17495: fortune teller (300290, left door x 240000). She says **1 Mark of Heroism is missed**. For 700 CP she names the place: **"You missed the mark in the Barness Hills Dungeon."** I read the answer, then rewound to before paying and chose No. Gold kept, and the fact is known.
- f18179: candy boy (300300 2F, via 300130 x 6400): only small talk at this point.
- f19974–21119: **candy store 300210** (300110 x 80000). The Old Lady's sold-out scene ("a boy bought ten") starts the quest.
  - **Capsule machines:**
    - The 20 CP machine inside (x ≈ 36000) is the Merrin's Staff machine. Two plays (the second by accidental ent mashing) won **[Merrin Card]** and **[Mentama Drop]**.
    - The 200 CP machine outside (300110 x ≈ 81500, by the store door) is the Legendary Sword machine. One play won **[Kitty Ears]**.
  - Store drawer 300210601 (77600, 35200) → **[Mystery Juice] → Sana** (Max MP +4, now 108).
- f25727–27100: candy boy again. The trade talk: he wants a rare sweet.
- f29643–30891: **item shop 300250** (300120 x 102400). Bought **Revival Drink** (350) and **4 Better Herbs** (18 each). Also bought 2 Herbs (20 CP) by accidental ent mashing. Sold Cloth Armor for 140.
- f31790: **bakery = 300270** (300120 x 220800, by the city gate). The rescued couple's wife gives **[Handmade Tart]**.
- f35417–36874: candy boy trades two tarts for five sticks → **[Taiyaki Sweet Sticks]**. Arche: "let's get back to Tonkiness!"
- f38760–39750: inn 300240. Rest 8 CP, then save to slot 13.

## Party at the end
- **Arche:** Lv7 (Lv cap 9), 197 HP, 31 MP. ATK 47 / DEF 49 / Spirit 18 / Resist 25. Gear: Iron Sword, Thick Leather, Woven Gloves; Accessory empty.
- **Sana:** Lv5, 104 HP, 108 MP. DEF 33. Gear: Iron Staff, Leather Armor, Chest Sash, Green Smock.
- **Items:** Herb ×3, Better Herb ×5 (by the shop's "You have"), Revival Drink, Magic Candy ×3, Mentama Drop, Merrin Card, Kitty Ears, Underwear, Taiyaki Sweet Sticks (key item).

## Upgrades
- **Taken:** Thick Leather, Woven Gloves, Chest Sash, Power Drop, Guardian Drop, 2× Mystery Juice, Revival Drink, Better Herbs.
- **Skipped:**
  - **Shortsword** (2800): not affordable.
  - Mahogany Staff (2900), Bronze Armor (2600), Rondel (1050, Stella's dagger), Longsword (5500).
  - Arche's **Accessory is still empty**: neither shop sells one.
  - Inn top-left closet (Better Herb) and 2F closet: not worth the frames.
  - Item-shop 3F closet: skipped.

## Shop lists (EN-old)
- **Weapons store 300220** (Old Man; Buy has an "On who?" equip compare + a Yes/No confirm): Shortsword 2800, Longsword 5500, Iron Staff 350, Mahogany Staff 2900, Rondel 1050, Thick Leather 950, Bronze Armor 2600.
- **Item shop 300250** (the Buy list buys at once on `ent`): Herb 10, Better Herb 18, Venom-Away 7, Revival Drink 350, Letter Opener 180, Cloth Armor 280, Honey 24, Chest Sash 78.
- **Sell prices:** Iron Sword 390, Thick Leather 475, Woven Gloves 90, Iron Staff 175, Leather Armor 280, Chest Sash 39, Green Smock 42, Cloth Armor 140. Sell asks Yes/No.
- **Inn:** 8 CP.

## Game facts and walkthrough corrections
- Fortune teller: the count is free; the location costs 700 CP. EN-old answer here: the missed Mark is in the **Barness Hills Dungeon**.
- The Status screen shows "Marks Gained 6 / 20" while the Lv cap is 9 with 7 Marks. Maybe the MoH 7 count lags, or the counter is something else.
- The sweet-stick quest order is: **candy store first** (sold-out scene) → boy → bakery tart → boy. The boy only makes small talk before the candy store scene. The bakery is 300270 in the east district by the gate, not "city entrance" from the west.
- **Capsule machines, corrected:**
  - The 20 CP machine is INSIDE the candy store; its grand prize is a **Merrin's Staff**.
  - The 200 CP machine is OUTSIDE (Legendary Sword).
  - After each play: "Should I try again?" defaults to **Yes**.
- The weapon shop asks for confirmation (equip compare, then Yes/No). DRIVER.md's "ent buys at once" holds for the item shop only.
- Items do stack in this save's menu (Better Herb x3), unlike mechanics.md §8 "never stack".
- Chest Sash: 0 DEF on both girls. Woven Gloves: DEF +2.
- V on a shop counter talks to the clerk, even when a barrel is closer.
- Walking from a standstill: the first ~6 frames of a press move nothing, then there is ~3500 lx of slide. Short `R*3` / `L*3` taps do nothing; use search for door alignment.

## Named state
None new. The flags-store lookup for an object handle needs a loop, and `rd eval` has no comprehensions (`sum` rejects a generator), so I could not check a container's opened state.

## Tool gaps
- `rd ck drop` twice preceded a bunker session rebuild ("it had no live savestate"; "not admitted — 4 rd sessions … 275 MB free"). One rebuilt command ran a play for only 1 tick.
- `rd tap ent --until control` presses `ent` once more after the dialogue ends, which swaps the leader (Arche → Sana). The same mashing auto-accepts Yes/No prompts: it paid the fortune teller (undone by rewind), played a capsule machine and bought Herbs. I switched to a script that stops at a Yes/No.
- I could not read object opened-state from `rd eval` (see Named state).

## Repo edits
None. No commits. Scratch helper script: /tmp/claude-1000/-opt-src-OpenSummoners/07101f4a-73ae-4a29-82db-8e98499aca28/scratchpad/adv.sh.

