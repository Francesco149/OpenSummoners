# probe story-day9a

run `story-d9a`; accepted 2026-11-19 02:20

check PASS (tools/drive/tasks/story-day9a.md)
verify MATCH (+ real device)
filed tests/scenarios/story-day9a/trace.jsonl (209786 frames)

## driver's report

# story-d9a: Day 9 school → barn (Sana) → both moms → main road (rose bush burnt) → White Rose Inn (rest + save) → Chartreux shops → Mayberk Manor (Rowen intro, grandfather-portrait scene)

**Run:** `story-d9a`, created with `RD_HOST=bunker ./rd new story-d9a --from story-d8i` (replay 172512 frames, 486 s). Start f172512 in 230140, `script` 1, party 1 (Arche alone).
**End:** f209786 in **310130** (Mayberk Manor hall, upper floor, far left, lx 12808 ly 25700). `control` 1, party 7, Arche leading. `story` 1420. That is ~37.3k frames, stopped near the ~40k budget.
Every rd command used `RD_HOST=bunker`. Nothing ran on the Windows box. No DISK LOW.

**Gates:**
- `rd check` **PASS**: step 1 at f173867, step 2 at f174049.
- `rd verify` **MATCH** at f209786 (rng f9099f5c, semv 812fe8a01c38e895).
  - Verify first reported "game state differs, frame + RNG equal: the SESSION drifted". It rebuilt the session by a clean replay (513.6 s), and that replay MATCHes.
  - The drift dump (`verify-drift-f209786`, bunker batch 20261118-180521-rd-verify-dump) shows **0 differing dwords** at f209785/209786.
  - So the drift sits in something the dump does not cover. Likely suspects are the many `rd undo` / `ck load` / `rewind` restores in this run.

**End save:** **`a2a16253c9bb`** (f192839): White Rose Inn 265165, slot **22** (empty before; slot 21 = the Day 8 save left untouched). Continue with `rd new NEXT --from save:a2a16253c9bb`. No later save point was passed (none in Chartreux on the route; the manor has none seen).

## Route (frames)
- f173866: school scene done (Stella's invitation). Control, party 5 (Arche + Stella).
- 230140 door x 22400 → 230130 → zone 1 (83200) → 230110 → zone 3 (224000) → **230120** (barn yard) → barn door (51200) → **230125**.
- Sana stands at x ≈ 34400 in the barn. X facing her → talk → f176019 control, **party 7**.
- Back out: 230110 zone 1 (12800, ↑) → 225130 → 225120 → 225110 → ↑ (9600) → Tonkiness 210110 (f180038), by searches.
  - 225130 has a 12800-tall step at x ≈ 70400: needs `C*30`.
- **Arche's mom:** 210120 → 210140, middle floor (kitchen, ly 32800, mom at x ≈ 65000). **X did nothing; V talked** → permission at f181508.
- **Sana's mom:** she is NOT in 210180 (Sana's house front room from 210120 zone 3, where only the "Old Lady" grandmother is). She is in **210190** = 210110 zone 5 (214400) → 210130 → zone 3 (60800) → 210190, mom at x ≈ 38000. X (facing right) → permission, f184839. `story` read 1420 there (1410 at the run start; I did not pin which event set 1420). It stayed 1420 through the manor scenes to the end.
- Town → road west: 210110 zone 1 (12800) → 260110 → 260120 → 260130 → 260140 → 260150 → 260180 → 260190 → zone 3 → 265170 → **265160** (f190032), all by `rd search`.
- **265160 rose bush** (code 70400, ten pieces at x 202800–211200, y 20400–38400; the road west is blocked at lx 215216):
  - Stella leading (`ent` ×2), fire mode.
  - ↑+X burns the upper pieces. **↓+X (short low flame) burns the low pieces**; →/←+X and Fire Wall do not.
  - Passable at f191363 with 8/10 pieces burnt (the two left are the top ones at y 20400–22000).
- **White Rose Inn 265165** (door x 99200):
  - Innkeeper woman = X facing right at lx 60016. Rest **7 CP** (6195 → 6188), then a black screen that waits for `ent`, then "Would you like to save?" Yes/No.
  - Saved in slot 22 at f192778 → endsave a2a16253c9bb.
- **White Rose shop** (Old Man, counter left; X at lx 27288 facing left): Fresh Milk 8, Better Herb 18, Venom-Away 7, Cloth Cape 520. **Bought 6 Better Herbs (108 CP).**
- 265160 → 265150 → 265140 → 265130 → 265120 → 265110 → **Chartreux 300120** (f198948).
  - 265140's west exit at x 0 is held by 51012 / 51051 foes. It needed a fight search with `zrest` in the cell, sword drawn.
  - 265120 needs dash-jumps (`L . L*8 LC*30`) over the water gaps.
- **Item store 300250** (counter keeper; X at lx ≈ 55700 facing right):
  - Page 1: Herb 10, Better Herb 18, Venom-Away 7, Revival Drink 350, Letter Opener 180, Cloth Armor 280, Honey 24, Chest Sash 78.
  - Page 2: **Venom-Away S 40, Holy Water 65, Magic Candy 120, Recall Lamp 500**.
  - Bought **Magic Candy ×4 + Recall Lamp ×1** (980 CP).
- **Weapons 300220** (300110 x 220800; the bridge girl blocks, jump):
  - Keeper "Old Man" = code 50162 at x 63800. X worked only from lx 68432 (right of him, facing left). X near x 56–57k talks to the Boy 50120 instead ("we only sell [the trick sword] at night", Yes/No).
  - Page 1: Shortsword 2800, Longsword 5500, Iron Staff 350, Mahogany Staff 2900, Rondel 1050, Thick Leather 950, Bronze Armor 2600, Bastard Sword 10500.
  - Page 2: Battle Sword 18000, Magic Dagger 9700, Scale Armor 4800, Iron Armor 9800, Bronze Gauntlets 2800.
  - Day stock: no Ghost Blaster.
  - Bought nothing (see Upgrades).
- 300110 ↑ x 126400 → 300130 → ↑ x 144000 → **310110** (Mayberk Manor title card + garden talk, control f205365) → zone 2 (387200 ↑) → **310120**.
  - Rowen introduction scene ("we have a little time before dinner … show us around"). Control at f207180.
- 310120 zone 2 (67200) → hall 310130 → zone 4 (99200) → **310140 ground floor**.
  - Walking left triggers the **grandfather-portrait scene** (Viscount Nicholas Mayberk), at f208227.
- Hall 310130 stairs (see below) → upper floor, far left (f209786). Stopped there.
  - Not yet done: Stella's room, "talk to everyone", the evening trigger.

## Party at the end (f209786)
HP / MP: Arche 199/216, 39/39; Sana 146/149, 161/161; Stella 178/189, 132/132. All Lv10 (save screen + inn status).
- **Arche:** Org Slayer, Thick Leather, Leather Cape, Kitty Ears.
- **Sana:** Mahogany Staff, Leather Armor, Chest Sash, Green Smock, Anti-Poison Ring.
- **Stella:** Lady's Dagger, Woven Gloves, Salamander Scale. Still **no body armour**.

**Gold:** 5139 CP.

**Items (seen):** Better Herb ×9 (the bag showed one ×3 stack + six singles), Magic Candy ×4, Recall Lamp ×1, Revival Drink ×1, Venom-Away S ×2, plus the spares Iron Staff, Rondel, Chest Sash, Underwear, Orange Beret.

## Upgrades taken / skipped
- **Taken:** 6 Better Herbs, 4 Magic Candy, 1 Recall Lamp. The catacombs are long; the lamp is the way out after the MoH 13 / Pocketwatch part.
- **Skipped: body armour for Stella.**
  - The bag had none spare: "Thick Leather You have 1" in the shop = the one Arche wears.
  - Options were Bronze Armor 2600 (wearability for Stella unchecked) or Scale Armor 4800.
  - Catacombs step 2 gives **[Scale Armor]** free. I kept the money toward Ghost Blaster (12,500, night), though 5139 CP is far from it.
  - The next leg can still buy Scale Armor / Bronze Armor at 300220 if wanted before the graveyard.
- **Skipped:**
  - Calming Cocoa weight cave (260165): needs a Stay There! puzzle.
  - Heroic Cape 12,800 and Ghost Blaster 12,500: not affordable.
  - Barn chests.

## Game facts / walkthrough corrections
- **Day 9 order confirmed:**
  - The school scene gives Stella (party 5). Sana is in the **barn 230125** (230110 → 230120 yard → barn door x 51200).
  - Both moms must be asked: Arche's mom in the 210140 kitchen, Sana's mom in **210190** (not 210180).
- **Talking:** X talks to most NPCs, but **Arche's mom answered only to V**. Shop keepers answer X only from a particular side or distance:
  - item store: from the left, about 2.5k away;
  - weapons: from the right side;
  - X picks the nearest NPC in front.
- **Stella's comment:** she "can easily burn through that rose bush that tangles the main road" = 265160. It burns piece by piece: ↑+X for high pieces, **↓+X for low ones**.
- **White Rose Inn = rest 7 CP + save offer.** After resting the screen stays black until a key is pressed.
- **Mayberk Manor hall 310130 stairs:**
  - Ground → the right-hand flight leads to a landing at (≈177000, 58400). Walking left from the landing goes back DOWN.
  - The middle floor is reached by jumping up from the landing (search path `C*12 RC*24 D*4 UR*32 D*8 C*12 LC*10`).
  - Middle → upper is the far-left stairs (search path, ends lx 12808 ly 25700).
  - A maid NPC on the ground floor at x ≈ 158000 blocks walking; jump her.
- **310140 ground floor:** the Viscount-portrait scene triggers on walking left from the hall door (the cook stands at the left).
  - The upper floor (with the storage chest = probably Stella's room) was not reached.
  - 310140's ladder at x ≈ 111000 did not climb with `U*40`.
- Chartreux item store page 2 confirms the walkthrough's Day 9 additions: Venom-Away S, Holy Water, Magic Candy, Recall Lamp.
- The weapons boy hints the night-only sword.

## Named state (run vars)
- `p0 p1 p2 m0 m1 m2 pmin e0..e8`: copied from story-d8i.
- `hsum` (sum over 32 slots) is too long for the live server (2243 tokens > 598). Not defined.
- `zrest` in the search cell (`(room, lx // 1600, ly // 1600, zrest // 10)`) made exit-holding fights searchable.

## Tool gaps
- The **Bash harness moves calls over ~3 min to the background**: `rd rewind` / `rd look --at` each replay for minutes.
  - `rd look --at F` replays from boot when F is not a live checkpoint.
  - `rd rewind --to F` took > 3 min.
- **No "talk target" probe:** finding where X / V talks to a counter keeper took many tries. A var with the NPC X would pick (or the nearest NPC + facing) would help.
- `actors()` is not mapped for SotES, so foe totals over 32 slots do not fit in one expression.
- The `sword` var lagged: it read 0 after `Z` until a later frame.

## Repo edits
None. No commits.

