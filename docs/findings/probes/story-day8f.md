# probe story-day8f

run `story-d8f`; accepted 2026-11-18 09:50

check PASS (tools/drive/tasks/story-day8f.md)
verify MATCH (+ real device)
filed tests/scenarios/story-day8f/trace.jsonl (71259 frames)

## driver's report

# story-d8f: Fire Shrine → BOSS Great Scorpion → Kanon → Password Note → Korat Caves (MoH 10, steam lever, Power Drop)

**Run:** `story-d8f`, created with `--from story-d8e`, so it starts at f39413 in room 380135 with control, Sana leading and a 3-member party (`party` = 7). It ends at **f71259** in **370120** (Korat Caves, the lever/chest cave) with control, Arche leading and the sword drawn. That is about 31.8k frames. Every rd command used `RD_HOST=bunker`; nothing ran on the Windows box, and no DISK LOW was seen.

**Gates:**
- `rd check` **PASS**: step 1 at f39414, step 2 at f41091. It passed at f54971 and again at f71259.
- `rd verify` **MATCH** at f71259 (rng e20ec6d2, semv 5b146a5cdbcbe421, fb c442da465beb5b39).

**End save:** **4ce2d13efdf6**, filed at f54971. It is the Warm Breeze Inn, **slot 21 "Korat Village"**, with all three members at Lv9. It is taken after the Password Note, before the Korat Caves. No save point was passed in the caves. To keep the later progress (MoH 10, lever, Power Drop), continue with `rd new NEXT --from story-d8f`.

## Route (frames)
- **f39626:** Sana cast Heal Water (`n2`, hold X ~140 frames) on herself, 53 → 139 HP. A second cast at ~f39846 healed Arche to 209.
- **Shrine room graph,** read from memory (area table `s32(world+0x103c)`, 336-byte records, zone table +0x1c):
  - 380110: z1 → 360130, z2 → 380130, z3 → 380120
  - 380130: z1 → 380110, z2 → 380135
  - 380120 and 380135: dead ends
- **f41090:** left 380135 through the arch door (x 16000, press up) → 380130.
- **f42034:** 380130 walk-in at the left edge (x 0) → 380110 (457600, 23200).
- 380110: walked west along the upper corridor and dropped down the shaft (x ≈ 350–370k) to the bottom (y ≈ 60–65k). Then went east to the lower-right pocket. Two Poison Myconids and two Poison Mothbees hold it; Arche was put to sleep.
- **f44409:** zone 3 (412800, 67200) → **380120**. This is the boss room; the scene starts on entry.
- **Cutscene:** Kanon at the "fire elemental statue", then the Great Scorpion. **Fight starts f46238.** In the cutscene the boss says magic can't pierce its shell.
- **BOSS: Great Scorpion,** code 100178, **2354 HP** (char slot 5).
  - Arche's sword does only 2–12 per hit, and she was quickly poisoned and disarmed (lpose 118).
  - **Stella's Phoenix** (`n3`, hold X ~105 frames, 4 MP) does **~110–135 per cast**.
  - Method: Stella leading at x ≈ 44000, repeating `(X*105 .*30)`. Sana's AI kept healing. One Magic Candy went on Stella at f50535.
  - **Dead at f51019.** +421 CP; Stella and Sana reached Lv9.
- **f51318–f52579:** Stella, Chiffon and Kanon scene ("he was even more vulnerable to magic"). The game then moves the party to the Warm Breeze Inn by itself (360170): Ellie scene, then the pamphlet.
  - **f53611: "You obtained Password Note."** The password is: 'As the hot water flows from the mountain, so too flow the hot, jealous tears of Scotsholm to us!'
  - `story` stays **1320** through all of this.
- **f54418–f54971:** Rested (16 CP) and saved slot 21. endsave **4ce2d13efdf6**.
- **f56198:** Item store (360160). Bought **Magic Candy ×2** (240 CP).
- **360110 → cave 360120.** zone 7 is at x 441600 (dash momentum overshoots it, so walk back).
- **360120 → 360140,** zone 2 at x 28800, y 44800.
- **f59477:** sealed door (360140 zone 2, x 185600). The scene starts on arrival; Arche recites the password. **Door unlocked f59946.**
- **f60129: Korat Caves 370110.** Entry scene; control at f61055.
- **Korat Caves graph:**
  - 370110: z1 → 360140, z2 (150400, 102400) → 370130, z3 (252800, 44800) → 370120, z4 (214400, 25600) → 370115
  - 370120 and 370115: dead ends
  - 370130 → 370140 / 370150 → … → 370210
- **f63431:** 370110 → **370115** (up-right tunnel; three Red Slimes nearby had to die first). **f63678: Mark of Heroism (MoH 10).** The money cap is now **30000**.
- **370110 → 370120** (the right exit). It needs the full loop: west over the lava bridge (platform 70120), past the steam vent at x 179200 (search timed it), down the west side, east along the bottom, then up the east slope. **↑ while idle at the right edge** fires it (f67086).
- **f67745: lever 370120512** (code 70216, at (60800, 163200), bottom-left) flipped with **V**. Vent 370120511 next to it stopped (state 1). Dragon Pups (code 51261) spawn around it.
- **Bottom-right chests in 370120** (cleared a Salamandra 51043 and two Dragon Pups first):
  - **f70259:** chest 370120503 → **[Salamander Scale]**
  - **f70408:** chest 370120502 → **[Power Drop]**, used on **Arche** at ~f70800 ("Arche's Attack increased by 1")
  - Chest 370120501 (20800, 166400, bottom-left) was not opened. It is probably the poison chest from the walkthrough.
- **Stopped at f71259** in 370120 (212752, 159100) with control and everyone alive. The next boss (New Age Myconid) is still several puzzle rooms away.

## Party at the end (f71259)
- **Arche:** **Lv9**, 209/209 HP, 33/33 MP. Org Slayer, Thick Leather, Leather Cape, Kitty Ears. +1 Attack from the Power Drop.
- **Sana:** **Lv9**, 132/145 HP, 113/150 MP. Mahogany Staff, Leather Armor, Chest Sash, Green Smock. She learned **Heal Water II** (already in Book A).
- **Stella:** **Lv9**, 107/182 HP, 95/114 MP. Rondel, Woven Gloves, still no body armour. She learned **Disintegrate**.
- **Gold:** 1565 CP (cap 30000).
- **Items:**
  - Healing and MP: Better Herb ×5, Venom-Away S ×3 (pouches), Hot Springs Bun ×3, Sweet Sticks ×5, Handmade Tart, Holy Water ×2, **Magic Candy ×3**, Mind Chocolate, Revival Drink.
  - Other: **Recall Lamp**, **Salamander Scale** (not inspected), Password Note (special item).

**Upgrades skipped:**
- **Stella body armour:** Korat's Scale Armor costs 4800, which I can't afford.
- **Salamander Scale:** I did not look up what it is or equip it. It is not in Special Items, and Items pages 1, 4 and 5 don't show it, so it is probably on page 2 or 3 (unchecked).
- **Chest 370120501:** left closed as the likely poison trap.

## Shop seen
Korat item store, unchanged from d8e: Better Herb 18, Venom-Away S 40, Holy Water 65, Revival Drink 350, Magic Candy 120, Recall Lamp 500, Leather Cape 1100, Magic Earring 4200.

## Named state (run vars)
- `p0 p1 p2 pmin`: party HP, by party slot (Arche, Sana, Stella).
- `e0..e7` and `x0..x7`: char-slot HP if a targetable hostile, and x positions.
- `ehp`, `enear`: sums of those.
- `bhp`: the boss's HP (slot 5 in 380120).
- `lev`, `vent511`, `ch502`, `ch503`: 370120 object states via `s32(s32(world+0x11e0+4*slot)+0xc)`.
- **Gotcha:** `pmin < 150` in an avoid is always true, because Sana's max is 145. My search reported "one cell only" for that reason.

## Game facts and walkthrough corrections
- **Fire Shrine boss route:** after the staff, go back out 380135 → 380130 → 380110. Drop down the 380110 shaft (x ≈ 350–370k) and go east along the bottom to zone 3 (412800, 67200), which leads to **380120 = Great Scorpion**. That is "drop, go RIGHT", not "go left".
- **Great Scorpion:** "magic can't pierce my shell" is a bluff. **Physical attacks barely hurt it (2–12 per hit); Stella's Phoenix does ~110–135.** About 17 Phoenix casts plus one Magic Candy won it in ~4800 frames.
- **After the boss the game teleports the party to the inn.** The Ellie and pamphlet scenes play there with no walking. You get the **[Password Note]** (a special item).
- **Sealed door:** 360140's right door (x 185600) → 370110 (Korat Caves). The scene starts on arrival; no input is needed.
- **MoH 10** is in 370115 (the up-right tunnel of 370110), floating at (38400, 20405). A full jump (`C*30`) grabs it. The money cap became **30000**, not the walkthrough's 40,000. The cap numbering is off by one MoH vs [DK] (d8c also saw MoH 9 → 20000).
- The "right lever → steam stops" in 370120 is at the **bottom-left** (60800, 163200). V flips it (state 1).
- **370120's bottom-right chests gave [Salamander Scale] + [Power Drop],** not the walkthrough's [Lady's Dagger] / [Tough Drop]. Those may be elsewhere in the caves.
- **Shopkeepers must be talked to while FACING them.** The Korat item-store lady (char code 50281) stands at x 84600. Talking to her from the left side works only facing right.
- **A target lock-on** (a "TARGET" marker under an enemy) keeps the leader facing that enemy. Walking the other way becomes a back-step (lpose 107). Examining / V toward a chest fails until the enemy is dead.
- **Item use from F1 is a timed cast bar** (~200–250 frames) for Magic Candy and Power Drop too.
- A pending level-up makes F1 show "Level-up Info" first; Enter closes it.

## Tool gaps
- **`--until` with a 30+ term expression** hit the live server's 598-token limit (`control or ehp > 0`).
- **`toward()` walks overshoot by up to ~14k** when the leader starts from a dash or slope. Walk-in zones were passed because of this. Short `R*N` presses often only turn the leader (no movement for ~10 frames).
- **No way to tell from state that a lock-on target is held.** A `lockon` var would explain stuck facing.
- **Searches got slow** (150–260 ticks/s in fights on bunker), so 150–200 s searches explored very little.
- **`rd map` misses slopes and ledges.** In 370110 it showed walls where the leader walked.

## Repo edits
None. No commits.

