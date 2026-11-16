# probe story-day7

run `story-d7`; accepted 2026-11-16 19:15

check PASS (tools/drive/tasks/story-day7.md)
verify MATCH (+ real device)
filed tests/scenarios/story-day7/trace.jsonl (314753 frames)

## driver's report

## story-d7: Day 6 finished; Day 7 got as far as the Weathervane road (260180). Stopped when the budget ran out, before the Lead Kobold.

**Status:** the run is at **f314753**, about 43.6k frames past the start (f271179). It stands in room **260180** at lx 8048, ly 29600: Arche 123/188 HP, Sana ~60/100, 1957 CP, sword drawn. Exit zone 2 (walk-in at (0, 19200)) leads to **260190**, the sign room (↑ Weathervane / Loch Carudale). Two hostiles are still alive: h1 = 42 HP and h2 = 39 HP, slots 1–2. The slot codes before the last fight were Mothbee 51025 (slots 0–1) and 51011 (slots 2–3), so these are probably a Mothbee and a 51011. `zrest` was 0 at the stop. **I did not run `rd check` or `rd verify`**; both are needed before filing.

### Route reached (frames)
- **f271359–272021:** a post-boss cutscene in 320165.
- **f272148:** reached 320170. Its chest (44800, 32000) opened at **f272593**; this should be the [Topah Herb].
- **f273154:** walking out of 320170 only returns to 320165, and `story` stayed at 1140. There was no event, and the cave has no short exit. Walking back, respawned Kobolds and Myconids in 320158 took Arche to 39 HP and Stella to 4.
- **f278681: F1 → System → Give Up → Return To Safety.** This took 10 % of the money (2339 → 2106) and put the party in **270135**, the herbalist's back yard right outside the cave, at full HP. Walking to its zone 1 (12800, ↑) played the Sana's-house cutscene: **`story` 1140 → 1150**, room 210190, Arche alone.
- **f286663:** ↑ at 210190 zone 1 (back door, x 12800) ends Day 6. The front door (x 64000 → 999999) says "The door is locked." Narration → "Would you like to save?" → saved at about f287000.
- **Day 7:** the class scene ran with `story` = 1200. Talked to the Class Prez at f293390 (Chiffon gag) and to **Colm at f293865** (sweet sticks; he stands at the far right of 230140, about lx 81000). Sana rejoined, so `party` = 3.
- **f296125:** Arche/Sana scene on leaving the school (230110). Then 225130 → 225120 → 225110 → Tonkiness (f300589).
- **Money from the moms:** Sana's mom gave **+30 CP at f302155** (210190, X while facing her). Arche's mom gave **+30 CP at f304925** (upstairs in 210140, right side). Arche's dad runs the ground-floor shop in the same house.
- **f308872:** Tonkiness inn (210150): Rest 3 CP, then Save.
- **f310041:** left town LEFT, 210110 zone 1 → 260110. Then 260120 (Mothbees put Arche to sleep, `lpose` 117) → 260130 → 260140 → 260150 → 260180 (f313856).

### End saves (digests)
- **2ed9735ba893** (f288613): the end-of-Day-6 save.
- **d89ccc7f7e38** (f309307): Tonkiness inn, Day 7, after shopping. This is the one to continue from.

### Party at the end
- **Arche:** Lv6, "MAX: 8" on the Status page, which also shows "Marks Gained 5/20". That conflicts with the start note "cap 9 after MoH 6" — check it. Iron Sword, Leather Armor; Armor #2 and Accessory empty.
- **Sana:** Lv4, 100 HP / 88 MP, Attack 23. She rejoined with **no equipment at all**. Now she has the Iron Staff (bought), the Cloth Armor (Stella's old one, unequipped in the inventory) and the Green Smock (a costume, no stats). Both Good Child Drops went to her: the inventory said ×1 but there were 2, each +2 Spirit.
- **Items:** 8 Herbs, Revival Drink, Magic Candy, Guardian Drop (kept), Underwear.

### Upgrades taken and skipped
- **Taken:** Iron Staff and the Herbs. I sold the Bronze Sword for 190 CP.
- **Skipped:** Leather Armor for Sana (560), to save money for Chartreux. Better Herbs: Tonkiness does not sell them. The Lead Kobold, MoH 7, Cloth Gloves and Chartreux were not reached.

### Shop lists and prices seen
- **Arche's dad's shop (210140 ground floor):** Mentama Drop 5, Herb 10, Venom-Away 7, Pot 60, Taiyaki 6, Slime Bean 12. `ent` buys at once.
- **Tonkiness weapon shop (210160; owner at the right, about lx 63000):** Bronze Sword 380, Mattress Beater 155, Iron Dagger 450, Cloth Armor 280, Iron Sword 780, Iron Staff 350, Leather Armor 560. Unlike the item shop, it asks who will equip the item, shows a stat preview, and asks "Would you like to buy?" (Yes is the default). Selling also asks to confirm. Sell prices are half.
- **Inn:** 3 CP.

### Named state (this run)
- `ahp` / `nahp` / `shp`: HP in party slots 0 / 1 / 2 (Arche / Sana / Stella).
- `h0`..`h7`: per-slot hostile HP (slot in use and faction 2); `fh` = their sum.
- `tx` / `ty`: x / y of the first live hostile among slots 0–2. I meant to name the Lead Kobold's HP but never reached it.

### Walkthrough corrections and game facts
- **Step 11.7:** no event fires when you leave 320170. You must leave the cave. **Give Up is the fast way out:** it costs 10 % of the money, and the retry room 270135 is the herbalist's back yard.
- **Sana's house:** the day ends at the back door, not the front door.
- **Tonkiness** sells no Better Herbs.
- **Sana rejoins on Day 7 unequipped.**
- `ent` in free roam swaps the leader, so tapping `ent` through cutscenes can leave the wrong leader in control. Use `C`, but in free roam `C` jumps.
- Mothbee sleep powder holds Arche in `lpose` 117 for more than 300 frames, and mashing keys does not wake her.

### Tool gaps
- Searches dropped to 38–126 ticks/s in fights; a 4-way search took 800 s.
- The 598-token limit stops a `{toward(tx-lx)}` input from using a target expression more than twice, and stops `fh` from being used in `--until`.
- `rd scan` replays from boot, which took about 460 s.
- `rd text` does not capture dialogue that is still being typed.

---
model claude-opus-5-5, effort high, 292 turns, $9.33, 112 min

