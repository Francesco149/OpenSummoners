# probe story-day8e

run `story-d8e`; accepted 2026-11-18 09:42

check PASS (tools/drive/tasks/story-day8e.md)
verify MATCH (+ real device)
filed tests/scenarios/story-day8e/trace.jsonl (39413 frames)

## driver's report

# story-d8e: Korat shops → hot-spring scene → Mayor → Ellie → inn save → Fire Shrine (Mahogany Staff)

**Run:** `story-d8e`, from `save:b72717552038` (slot 19 loaded, control at f880). It ends at **f39413** in room **380135** (Fire Shrine, the chest room) with control, Sana leading. That is about 39.4k frames, so the budget is used up. The run stops **before the Great Scorpion**, which was not reached. Every rd command used `RD_HOST=bunker`; nothing touched the Windows box, and no DISK LOW was seen.

**Gates:**
- `rd check` **PASS**: step 1 at f881, step 2 at f3001.
- `rd verify` **MATCH** at f29406 (semv 2c86cc2396536d64).
- `rd verify` **MATCH** at f39413 (rng 501aa7b7, semv 1fc00d9741c3e045, fb bfdc154413f51457).

**End save:** **6700850a55ab**, filed at f29180. It is the Warm Breeze Inn Save menu, **slot 20 "Korat Village"** (made at f~29030). It is taken after Mayor and Ellie and the item uses, but before the Power Drop and the Fire Shrine. To keep the later progress (Power Drop, Mahogany Staff, Arche Lv9), continue with `rd new NEXT --from story-d8e` instead. No save point was passed after the inn.

## Route (frames)
- **f880:** loaded at the inn 2F. f1286 → town 360110.
- **f1613: item store / storage** (360160, door x 230400). Bought Better Herb ×3 and Venom-Away S ×3.
  - The weapon counter is in the SAME room (360160) but walled off. It is reached only by its own door, zone 3 (x 192000).
- **f3468: weapon store.** Nothing affordable.
- **f4947: 360150, food/souvenir store** (door x 89600).
  - First talk = the free-bun cutscene; talk again for the shop.
  - Bought Hot Springs Bun ×3.
  - Drawer 360150601 (57600) → **[Mind Chocolate]**.
- **f8358:** back at the item store; bought **[Recall Lamp]** (500).
  - My ent taps on the storage box (70220 at 71600) put the **Iron Sword into storage** (deposit). It is obsolete, so I left it there.
- **f9786:** equipped **[Kitty Ears] on Arche** (Atk 64→70, Def 54→60; it had been sitting unequipped in the bag).
- **f10734 → 360175** (inn ground-floor door). The hot-spring problem cutscene (Marina, Ellie, Ms. Sophia, Stella) triggers on walking in. Control came back at **f12933**.
  - Pot 360175602 (78600, behind the teacher) → **[Strongirl Juice]**.
  - Shelf 360175603 (38400) → **[Underwear]**.
  - Reached 360175's 2F via the inn's 2F door (360170 zone 3):
    - drawer 503 = 'Magical Girl Merrin 9/12';
    - closet 360175601 (74800) → **[Good Child Drop]**.
  - Used Good Child Drop → **Stella** (Spirit +1) and Strongirl Juice → **Sana** (Max HP 136→139).
- **360110 pot 360110601** (352200, behind the old woman) → **gold coins +100 CP**, not Hot Water Salts.
- **f18275: 360120** (cave) → **f18627: 360130** (east village).
  - 360180 (x 115200): closet 602 → **[Orange Beret]**; 2F chest 601 → **[Mystery Juice]**.
  - Pot 360130601 (219800) → **[Guardian Drop]**.
  - **360200** (x 182400; maid) → right walk-in → **360206 = Mayor's room**:
    - closet 601 → **[Tough Drop]**;
    - drawer 602 → **[Chest Sash]** (a 2nd one).
- **f21816–f24576: Mayor talk.** The spring source is behind a magically sealed door in the village cave, which opens with a password. The Mayor wrote it on a tourist pamphlet and gave it to a girl sightseeing. `story` stayed **1320** through all scenes.
- **f26291–f27105: Ellie** (inn 1F, right of the fireplace). Kanon is missing; she got a pamphlet from the Mayor and went sightseeing.
- **Item uses:**
  - Tough Drop → **Arche** (Def +2).
  - Guardian Drop → Sana (I did not see the confirmation message; the cursor was on Sana).
  - Mystery Juice → **Sana** (Max MP +2, now 134).
- **f29036:** saved slot 20 at the innkeeper. endsave **6700850a55ab**.
- **360190** (360130 x 291200): drawer 360190601 → **[Power Drop] → Arche** (atk 44→45).
- **f32530: 380110 Fire Shrine**, entered through 360130's far-right door (412800, 67200).
  - Way up = the shaft at x ≈ 348000–353000. It has stone step-ledges on the wall; climb them with jumps.
  - The upper corridor runs east to the walk-in (457600, 12800) → **380130 at f35925**.
  - A Red Slime and a Poison Myconid hold that exit.
  - **Arche reached Lv9** in 380130.
  - 380130's only other exit is the arch door (169600) → **380135 at f38218**. Arche and Stella were put to sleep there by Poison Myconid spores (about 500 frames with Arche asleep), so I switched to Sana to get through.
- **f38799: 380135** chest 380135601 (180800, 25600) → **[Mahogany Staff]**. Equipped on **Sana** at f39413: Atk 26→30, Spirit 45→53.
- **Not done:** drop / go left → **BOSS: Great Scorpion**. Next: from 380135 / 380130, find the way down; 380110's zone 3 (412800, 67200) → 380120 is the other unexplored exit.

## Party at the end (f39413)
- **Arche:** **Lv9**, 132/209 HP, 32/33 MP. Org Slayer, Thick Leather, Leather Cape, **Kitty Ears**. Also Tough Drop (+2 Def) and Power Drop (+1 Atk).
- **Sana:** Lv8, **53/139 HP** (heal before going on), 118/134 MP. **Mahogany Staff**, Leather Armor, Chest Sash, Green Smock. Also Strongirl Juice, Mystery Juice, Guardian Drop.
- **Stella:** Lv8, 150/166 HP, 79/110 MP. Rondel, Woven Gloves; **still no body armour**. Also Good Child Drop.
- **Gold:** 1232 CP.
- **Items:**
  - Healing and cures: Better Herb ×5, Venom-Away S (3 pouches, x3 each), Hot Springs Bun ×3, Holy Water ×2, Magic Candy ×2, Mind Chocolate, Revival Drink, Handmade Tart, Sweet Sticks ×5.
  - Other: **Recall Lamp**, Chest Sash (spare), Underwear, Orange Beret, Merrin Card.
- **Storage (360160):** Iron Sword.

**Upgrades skipped:**
- **Stella body armour.** Korat sells only Scale Armor 4800 / Iron Armor 9800, both unaffordable. Thick Leather 950 is only at the 340165 merchant.
- Weapons, which were unaffordable.
- Magic Earring 4200.
- The costumes.

## Shop lists (EN-old)
| Store | Location | Stock (CP) |
|---|---|---|
| **Item store** (Old Lady, + storage box) | 360160 right door, x 230400 | Better Herb 18, Venom-Away S 40, Holy Water 65, Revival Drink 350, Magic Candy 120, Recall Lamp 500, Leather Cape 1100, Magic Earring 4200 |
| **Weapon store** | 360160 left door, x 192000 | Longsword 5500, Bastard Sword 10500, Mahogany Staff 2900, Lady's Dagger 4300, Scale Armor 4800, Iron Armor 9800, Bronze Gauntlets 2800 |
| **Souvenir store**, p1 | 360150 | Hot Springs Bun 30 (200–250 HP), Wooden Sword 850, Hot Water Salts 240, Red Clothes 68, Red Hat 48, Brown Dress 130, Red Hairband 30, Brown Smock 125 |
| **Souvenir store**, p2 | 360150 | Brown Beret 75, Red Outfit 160, Red Ribbon 80 |

## Named state (run vars)
- `pa ps pt pmin`: party HP.
- `o3/o5/o15/o19/o20`, `q11/q12`, `m0/m4`, `pd`, `gpot`, `pot601/602`, `drw`, `mch`: per-room object states, `s32(s32(world+0x11e0+4*slot)+0xc)`. The slots come from `rd zones --all` (the handle column = the loot flag).

## Game facts and walkthrough corrections
- **Pots (70302/70304) open by being HIT with the sword drawn** (X slash / Wide Slash), not by V. V and an undrawn X do nothing. This contradicts earlier reports that said pots are scenery.
  - Their hitbox sits LEFT of the actor x (offset −18 px).
  - Drawers, shelves and closets (70363/70364/70353) open with V.
- **Korat building map:**
  - 360160 = item store (+ storage) and weapon store, as two walled-off halves with separate doors.
  - 360150 = food/souvenir store.
  - 360120 = cave east (also its door → 360140 and → 360125).
  - 360130 = east village: 360180 1st building; 360200 → 360206 Mayor; 360190 Power-Drop house; 380110 Fire Shrine at its far right.
- The walkthrough's "pots behind Colm/Leon [Hot Water Salts]" = the town pot 360110601, which gave **gold coins (100 CP)** here.
  - "Next bldg (Mayor) [Tough Drop], [Chest Sash]" = the Mayor's room 360206 (closet + drawer), confirmed.
  - The inn "2F far right 'Merrin 9/12', closet [Good Child Drop]" is in **360175's 2F** (reached from the inn's 2F door), not in the inn proper.
- **Mahogany Staff** = Fire Shrine **380135** chest (380110 → shaft up → 380130 → arch door 169600 → 380135).
- **Poison Myconid** puts the party to sleep (sleep lasted >450 frames on Arche; status "Arche has fallen asleep.").
- Shop dialogue flow: after closing the Buy list it says "Thank you!", then "Will there be anything else?" with Buy/Sell; C → "Come again" closes by itself.
- An `ent` mashed after "Thank you" REOPENS Buy, and on the storage box it deposits items.

## Tool gaps
- **Bunker throughput** dropped to ~146 ticks/s late in the run, so 120–150 s searches explored very little. A session rebuild took 55 s.
- **Searches report "one cell only"** when the leader is asleep or stunned (lpose 117) or held by a fight. Nothing in the output says the leader can't act.
- **`--score` with `--apply-best` drifts past the target:** a score on lx overshot the 380130 door by 24k and wasted ~1100 frames. Searches also wandered back to far-left ledges unless lx was bounded in `--avoid`.
- **`control` is true while the F1 menu is open**, so `--until control` cannot detect menu closure.
- **`rd text` output in a `tap` is stale relative to what the final press did.** The inn Save got re-selected that way, which led to an "Overwrite?" prompt.
- Comprehensions (`for i in range`) are not supported in expressions, so there is no compact "nearest foe" expression.

## Repo edits
None. No commits.

