# probe story-herb-farm

run `story-hf`; accepted 2026-11-16 01:38

check PASS (tools/drive/tasks/story-herb-farm.md)
verify MATCH (+ real device)
filed tests/scenarios/story-herb-farm/trace.jsonl (219262 frames)

## driver's report

## story-hf: out of the dungeon, past the herbalist and into the Herb Farm Cave. Stopped in its third room (320120) when the budget ran out; Baby Mage not reached

**Status:** `rd check` PASSES on tools/drive/tasks/story-herb-farm.md (step 1 at frame 779, step 2 at frame 3001, the forbid never fired). `rd verify` gave **MATCH at frame 218554** (rng ce05be72). After the verify, a `--apply-best` search added 708 frames. The movie now ends at **f219262** in room 320120 (lx 67208, ly 134880, low in the room): Arche has 115/185 HP, the sword is drawn, and 544 enemy HP is still alive in the room's slots. That tail has not been verified. It is a plain search path, so it should replay the same way, but run `rd verify story-hf` again before filing. Run length: about 45k frames past the start (174304).

### Route reached (frames)
- **f174343:** control in 290230.
- **f174374–175797:** menu work. Healed Arche with a Better Herb (68 → 172), put Cloth Armor on Stella, and used the stat items listed below.
- **f176643:** 290230 zone 1 (x 19200, ↑) → 290210.
- **f178735–179954:** walked to 290210's left exit (x 0, y 38400, walk-in), but 2 Yellow Slimes held it; after a fight search → 290205.
- **f180615:** → 290200 (its exit is top-left, x 0 y 12800).
- **f182361:** → 290110.
- **f183099:** → 280170.
- **f183313:** → 280160.
- **f184232:** → 280115.
- **f186258:** 280115's zone 3 (x 224000, y 38400) → Barness 270120. Arche, Chiffon and Stella talk about the wind.
- **f187235:** an automatic scene in the Mayor's house (270170). "The crest is yours."
- **f188150–188404:** scene in 270110: Stella says they should get home.
- **f191662–194416:** in 260150, the evening scene with Colm and the boys. It ran about 1800 frames and the doctor news must have come during it, because letter No.12 "The Medicine's Missing?!" was there afterwards. That letter says to go to the herbalist in Barness.
- **f196839:** I walked on to Tonkiness (210110) before reading that letter, which wasted about 10k frames of walking there and back.
- **f201411:** Tonkiness inn (210150): rest 3 CP, then saved to **slot 10**. **End-save digest `9ebc4f357286`.**
- **f209641:** back in Barness 270110.
- **f210047:** the herbalist = 270110 zone 2 (x 313600, ↑, the first house from the east gate) → 270130.
- **f210332–210982:** herbalist scene. The cavern is "just out the door to the right".
- **f211484:** both characters healed to full.
- **f211762:** back door → 270135.
- **f212620:** ↑ at the cave mouth (lx ≈ 89500) → **Herb Farm Cave 320105**.
- **f213125:** Stella, as leader, climbed the crates onto the platform top (search with `--hold 30`).
- **f213401:** set Follow Mode → All Members 'Stay There!' in the menu.
- **f213801:** Arche broke the 3 crates.
- **f214289–214769:** Arche, sword sheathed, pushed the platform from x 38500 to 83200 (100 lx per frame).
- **f215000:** Stella run-jumped right off the platform → 320110 (zone 2 of 320105 at x 124800 y 6400). The whole party moved with her.
- **f216764:** → 320115 (side room).
- **f217693:** killed a Kobold, then the chest there = **Better Herb** (one).
- **f218514:** 320110 zone 3 (x 144000, y 115200) → **320120**.
- **f218554–219262:** exploring 320120 when I stopped.

**Not reached:** Memory Photo 4, the crescent wall / Stronggirl Juice, the torches, Good Child Drop, Revival Drink, MoH 6, the fountain, the Baby Mage, the Topah Herb. 320120's exits are zone 1 (x 22400, y 25600) back to 320110, **zone 2 at (288000, 121600)** and **zone 3 at (0, 96000)**. I had not yet learned which one is the main route. The room has many slimes, which slows searches to 200–350 ticks/s.

### Party at the end
- **Arche:** Lv6 (cap 7), 185 max HP. Iron Sword, Leather Armor. Armor #2 and Accessory are still empty.
- **Stella:** Lv4, 126 max HP. Cloth Armor (her armor slot was empty before).
- **Money:** 1363 CP.
- **Items:** about 5–6 Better Herbs, Magic Candy, Honey, 1 Good Child Drop, Guardian Drop, Mystery Juice, Underwear(M), spare Bronze Sword, Green Smock.

### Upgrades taken and skipped
- **Taken:** Power Drop → Arche. Good Child Drop ×1 and Strongirl Juice → Stella (max HP 122 → 126). Cloth Armor → Stella.
- **Kept for later:** the second Good Child Drop, the Guardian Drop and the Mystery Juice. My reasoning: Sana is the main caster and the guide names no one for the Guardian Drop.
- **Not filled:** Arche's Armor #2 and Accessory. Nothing in the inventory fits: Green Smock is "Cannot Equip" for both, and there are no gloves or rings.
- **Skipped:** the Barness store. We already had 5+ Better Herbs, and the Shortsword (2800) is more than we have. MoH 5 was also skipped: the Stella route out did not pass it. The Tonkiness shops were skipped too: the guide notes no gear upgrade there.

### Game facts and walkthrough corrections (EN-old)
- **Leaving the dungeon after the Stella boss:**
  - The reverse route is 290230 → 290210 → 290205 → 290200 → 290110 → 280170 → 280160 → 280115.
  - In 280115, **zone 3 (x 224000) goes to Barness 270120**; zone 1 goes to 280150.
- **Barness scenes:** walking into Barness starts the wind scene, then you are sent automatically to the Mayor's house (270170) for "the crest is yours".
- **"Boys, then the doctor":** the boys scene plays at **260150** (the junction) on the walk home. The doctor news comes in the same scene sequence, and letter No.12 then points at the Barness herbalist. **Do not walk to Tonkiness.**
- **"Barness herbalist (first house)"** = 270110 zone 2 at x 313600, the first house inside the east gate. Its room is 270130; the back door leads to 270135, and the cave is ↑ at lx ≈ 89500 → 320105.
- **Herb Farm Cave step 1 (320105):**
  - The "platform" is a tall wooden scaffold.
  - Stella, controlled directly, can jump crates → scaffold top.
  - Then set 'Stay There!' (F1 → Party Commands → Follow Mode → All Members). One F3 press from Follow gave 'Fight Freely!'.
  - Arche breaks the crates and pushes the scaffold right until it stops at x 83200.
  - Then Stella run-jumps right to the high exit, and the whole party transfers with her.
- **Chests:** the 320115 chest is a single Better Herb.
- **The Tonkiness inn** costs 3 CP on Day 6 and offers a save after the rest.
- **The Herb restored Arche to full from 98** (+87).
- **Arche reached Lv6** during the 260150 scene.

### Named state
- `ehp`: enemy HP summed over character slots 0–9 (in use and faction 2), defined in this run.
- **Zone listing** = `s32(A+0x274)*1e8 + x/100*1e4 + y/100`, where `A = s32(world+0x11e0+4k)`. Mind the digits: x/100 takes 4 digits. I misread x 304000 as 30400 once.
- **Stella as leader reads `lid` = 0**, not a 5001x id.
- **Not named (out of budget):** the follow-mode variable (`ram diff` was too noisy), the Baby Mage's HP and the torch flags.

### Tool gaps
- **No zone or door listing.** I had to type a 12-term eval each time.
- **`sum()` has no comprehensions.**
- **Searches crawl** (200 ticks/s) in rooms full of slimes.
- **`ent` in free roam switches characters,** so `tap ent --until control` toggled the leader. Use `C` to advance field dialogue.
- **The first R/L press after standing has about 20 frames of no movement,** so short presses don't move her.
- **One search found only 1 cell** even though walking worked.

---
model claude-opus-5-5, effort high, 292 turns, $9.54, 137 min

