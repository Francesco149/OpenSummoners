# probe story-day9f

run `story-d9f`; accepted 2026-11-19 15:16

check PASS (tools/drive/tasks/story-day9f.md)
verify MATCH (+ real device)
filed tests/scenarios/story-day9f/trace.jsonl (337196 frames)

## driver's report

# story-d9f: second Crypt Key → BOSS Master Ghost → MoH 13 → Thieves' Key → save (Day 9 done)

**Run:** `story-d9f`, made with `rd new --from story-d9e`. Every rd command used `RD_HOST=bunker`; no Windows box; no DISK LOW.
- **Start:** f293977 in 400180, control, party 7 (start state confirmed).
- **End:** f337196 in **310120** (Mayberk Manor), the Day 10 morning scene ("Arche, wake up"), `script` 1.
- Used 43.2k frames (budget 40k). The whole route through 4.8 is done.
- **Gates:**
  - `rd check` **PASS**: step 1 at f293978, step 2 at f296398.
  - `rd verify` **MATCH** at f337196: rng 3fb402a0, semv 4728ac1467014cd5, fb bdb12726973d7d02.
  - The mid-run verify at f315161 also matched. It first printed "SESSION drifted" with 0 differing dwords (evidence in `runs/story-d9f/verify-drift-f315161`), the same as story-d9e.
- **End saves:**
  - `38486ae7b57a`: f315141, slot 23 "Chartreux Catacombs" at the 400185 fountain (before the boss).
  - **`0f2d49481120`**: f337196. Slot 23 was overwritten by the save prompt after the Thieves' Key scene (see "Mistake"). Next leg: `rd new NEXT --from save:0f2d49481120`, or `--from story-d9f`.

## Route (frames)
1. **f293977–294643, 400180:** Sana `n2` Heal Water on Stella (+115). Stella `n3` Phoenix ×2 killed the Ghost Mage behind us.
2. **The 238000 "floor spike platform" (70430, object slot 8)** is a spiked elevator.
   - It moves between y 41240 (on the floor) and y 16000 all the time.
   - It is not the way to the key. The 70800s in 400180 (handles 511 / 512) are scenery.
3. **The "left door" is 400160 zone 4 (128000, 22400) → 400170.**
   - Route: from 400180, zone 1 (x 16000) → 400160 at the secret door.
   - Jump the tombstone (`LC*14`), go up the stairs left to the upper floor (y 16800), then drop to the zone 4 ledge and press ↑ (f296397).
   - Dash under the 400180 crusher from either side: hold the dash the whole way. Stopping, or the skid, puts you under it (−91 HP).
4. **400170 (f296437)** = "fall into skeletons": you land among 2 Skeletal Soldiers (51027).
   - Go left to the shaft (x 73600–86000) and **hug the RIGHT** wall while falling. You land on the y 96800 ledge (the crates 70303 there do nothing).
   - Walk down the slope right to two chests:
     - 70220 (174400, 128000) **[Crypt Key]**, f297314.
     - (180800, 128000) **[Guardian Drop]**, f297690.
   - Back up the slope, then drop down the shaft hugging LEFT to the bottom, where there are 2 Ghosts + 1 Ghost Mage.
     - A Stella Phoenix search killed all three (f300026; Arche reached **Lv12**).
     - Chest (80000, 144000) **[Power Drop]** (f300148). It opened only from lx ≈ 75800, facing right.
   - Exit: walk-in zone 1 at x 0 → **400175**.
5. **400175 (f301682):** bats (51062) and snakes (51052) — an Arche search killed them.
   - The barrel 70302 (5000, handle 400175601) gives **[Good Child Drop]**. Break it with an X slash, f302871.
   - Zone 1 (28800, ↑) → 400160 bottom-left (28800, 93600).
6. **400160 back to the secret door:**
   - Bottom corridor pits: `RC*50 L*8 R . R*6 RC*12 R . R*6 RC*12`.
   - Lower corridor right to (295600, 55200), then `R*20 .*10 LC*67` into the upper corridor.
   - Ghosts, a Ghost Mage and snakes chased us: about 4.5k frames of fights. Zone 5 → 400180 at f310008.
7. **400180 back east:**
   - Crusher dash (release first, then `.*20 R . R*80`).
   - The skeletons, Ghost Mages ×2 and Ghost had respawned. A **Magic Candy on Stella (73 MP)** plus Phoenix searches killed them all (f313244).
   - Keyhole 70217 (324800) → "You used Crypt Key." at f313691. You must stand at **lx ≈ 325400 facing left** with no enemy alive nearby.
8. **f314127 zone 3 → 400185 = the fountain room** (statue 70410 at 43200).
   - V → "Water flows from the fountain…" → Yes → full HP/MP → "save?" → slot 23 (f315141).
9. **Zone 2 (358400) → 400190 = BOSS.** Walking right fires the intro (f315821).
   - **Master Ghost: code 100185, 2343 HP**, actor slot 4.
   - Killed by Stella Phoenix score searches in four rounds, f316553 → f323206. Phoenix does ~90 damage per cast.
   - No Ghost / Ghost Mage adds were seen.
   - The scenes then give **[Pocketwatch]** (Mayberk engraving), f326457.
10. **400190 zone 2 (80000, ↑) → 400200 (f326894):**
    - Chest 70220 (30400, 73600) **[Crypt Key]**, f327790.
    - **MoH 13** = 70100 (27200, 60035): jump into it (`C*30` under it) at f328062. A celebration scene follows, about 850 f, with a full heal.
    - Keyhole 70217 (184000) from lx 184536 facing right (f329702). Zone 2 (224000, 73600, ↑) → **400210**.
11. **400210:** the "moaning" is **Alex** (NPC 50150), a lost ex-adventurer singing.
    - Stella gives him the Pocketwatch → **[Thieves' Key]** (f~334400).
    - Fade → **310120** manor scene → save prompt (slot 23) → "The Bonus Menu has been expanded." → Day 10 morning (f337196). `story` reads 1430.

## Party at the end (f337196)
- **Arche** Lv12, 255/255 HP, 41 MP. Learned Leaping Slash (not assigned). Power Drop (Attack +1) and Guardian Drop (Resist +1) used on her.
- **Sana** Lv12, 173/173 HP, 208 MP. Good Child Drop (Spirit) used on her.
- **Stella** Lv12, 218/218 HP, 156 MP.
- Gear unchanged from story-d9d (no gear found or shop on this route).
- **10802 CP** (+1992).
- **Items:**
  - Used: Magic Candy ×2 (both on Stella).
  - Left: Ghost Marshmallow ×3, Better Herb, Venom-Away S ×3, Revival Drink, Sweet Sticks ×2, Recall Lamp, Merrin Card, Doll Hand.
  - Key items: Thieves' Key, 1 spare Crypt Key (the 400200 one went into its keyhole; the 400170 one into 400180's gate).
- **Skipped:**
  - Arche's new ability Leaping Slash was not assigned: I did not try the Abilities menu.
  - The Doll Hand is still unknown.
  - No shop on the route.

## Game facts / walkthrough corrections (EN-old)
- **4.6:**
  - "Left door" = 400160 zone 4 (upper floor, x 128000) → 400170.
  - "Pit hugging right" = 400170 shaft at x 73600–86000; hugging right lands on the ledge that leads to [Crypt Key] + [Guardian Drop].
  - "Bottom [Power Drop]" is the bottom-left area (Ghosts + Ghost Mage). It exits only via walk-in x 0 → 400175.
  - "Left area barrels [Good Child Drop]" = 400175 barrel at x 5000. Break it with a slash.
  - 400175 ↑ zone → 400160 bottom-left, so the way back is the whole of 400160.
- **Examine (V) on keyholes / chests does nothing while an enemy is nearby (`foe` == 1) or while the leader is bound.**
  - Kill everything near first.
  - The keyhole's spot is about object x + 600.
- **4.7 fountain** = the statue in **400185** (behind the 335000 gate, walk-in right). It is not right of the fancy doors in that room.
- **4.7 BOSS:**
  - The fancy doors are 400180 zone 2.
  - Master Ghost (100185) has 2343 HP and no adds were observed. It drops coins (+920 CP) and the Pocketwatch in a scene.
- **Ghost Mages / Ghosts respawn on every room entry and chase across the room.** They were the main cost of this leg.
  - Mage binds stop all input, including V and party swap.
  - Phoenix (Stella `n3`, 4 MP, ~90 damage) is the only efficient answer we used.
- **400180's 70430 at x 238000** is an elevator / crusher that cycles 41240 ↔ 16000. Cross under it while it is up.
- **MoH pickup** starts a ~850 f celebration with a full party heal.

## Named state (this run's vars.json)
- `p0 p1 p2` party HP and `m0 m1 m2` party MP (room +0x4030 + 4i → +0xa64 / +0xa6c); `pmin`.
- `bh` Master Ghost HP (actor slot 4 in 400190). `moh` / `ks` object states (redefined per room).
- `e0..e5`, `g2..g5`, `h*`: actor-slot HPs (redefined per fight; several now read 0).
- Helper `actors.sh RUN N` (scratchpad) lists actor slots as code / HP / x / y / targetable, with x / y at `s32(s32(c+0x40)+4/8)`.

## Mistake
- In the manor scene after the Thieves' Key, `rd tap ent --until script == 0` mashed through **"Would you like to save?" → slot screen → Overwrite** (default Yes). The fountain save in slot 23 was replaced with the post-Thieves'-Key save.
- The result is harmless (later progress), but it is exactly the card's ⚠.

## Tool gaps / notes
- **`ck load` of an older checkpoint drops all later checkpoints**, and the save pool sometimes forces a full replay (~16 min on bunker) with no warning.
  - I lost `gf3` this way, and a load of `pasttrap` replayed from boot.
- **`--apply-best` can land on a doomed state** (a hit already in flight). The next search then reports "one cell only".
  - An avoid with a margin, or a `--hold` for apply-best, would help.
- **Bunker search speed varied from 37 to 900 ticks/s.**
- **`rd tap --until` has no guard for Yes/No or save prompts.** A `--stop-on-choice` would have prevented the overwrite.
- **A `--text` option for `play`** (or a scrolling text log) would save the many 60-frame waits spent reading chest messages.

## Repo edits
None. No commits.

