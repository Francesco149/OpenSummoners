# probe story-day11a

run `story-d11a`; accepted 2026-11-25 07:42

check PASS (tools/drive/tasks/story-day11a.md)
verify MATCH (+ real device)
filed tests/scenarios/story-day11a/trace.jsonl (546689 frames)

## driver's report

# story-d11a: Day 11 walkthrough steps 1–2 (Sophia's Letter → Ultimate Wheat → Ultimate Bread → Chartreux shopping + inn save)

**Run:** `story-d11a`, created with `rd new story-d11a --from story-d10e` (replay 491227 frames, 1647 s).
- **Host:** rd auto-picked **bunker**. I never touched the Windows box, so there were no ssh resets, no fallback and no DISK LOW.
- **Start:** f491227 in 230140 (classroom), in control.
- **End:** **f546689 in 300240** (the Dancing Piglet Inn, Chartreux), in control, leader Arche, party 7, HP / MP full after resting.
- **Length:** 55.5k frames past the start.
  - That is over the ~40k budget. I stopped at the end of step 2 and did **not** start step 3 (north of Chartreux, the first cave, the flower field / [Cleitcaithream]).
  - The walk Tonkiness → Barness → Chartreux alone cost ~30k frames.
- **Gates (at the end, f546689):**
  - `rd check` **PASS**: step 1 at f491228, step 2 at f491379, forbid clear (1991 s).
  - `rd verify` **MATCH**: rng 444ef278, semv bb74954ec3ab4a91, fb dab72f40b3b483e9 (1661 s).
  - I ran both at the end rather than at two thirds. That was the only point where gating made sense once I had decided to stop at the end of step 2.
- **End saves:**
  - **`704e90e24d19`** (final): in-game **slot 5** "Chartreux City, Arche LV14 / Sana LV13 / Stella LV13, 9:02". It overwrote "Mountain Cave LV3, 2:10".
  - `ea94dbe1a2f3` (earlier): **slot 3** "Barness Village 8:55", made at the Weathervane Inn. It overwrote "Town of Tonkiness LV2, 1:26".
  - Slots 1, 2, 4 and 24 are untouched.
  - The next leg can start with `--from story-d11a` (the run movie) or `--from save:704e90e24d19`.
- I looked at the end frame: `/mnt/ephemeral/opensummoners/drive/runs/story-d11a/view/look_f546689.png`.
- Session stopped (`rd stop`). Checkpoints: start letter tonk barness wheat innsaved pastvines chartreux bread shopped end.

## Route (frames)
1. **f491347:** 230140 door x 22400 → hallway 230130.
   - Wrong rooms first. 230150 ground-left is the nurse Ms. Remi (Rest / Buy Medicine; she rests the party and offers a save, which I declined). 230150 ground-right is the library.
   - **Staff room = 230140's UPPER floor.** Route: 230130 left stairs (walk left from the ground floor to the stair top at lx 9600, ly 32800) → `RC*14` onto the upper floor (ly 29600) → zone 4 (28800, 32000) → 230140 upper (entry at 22400).
   - Sophia = the blonde woman in blue, about lx 62000. The headmaster sits at the desk on the right.
   - Leaving 230130 by zone 1 before seeing Sophia triggers Stella's "Hold on a minute" and returns control. It does not block anything.
2. **f497833–501775:** Sophia scene → **"You obtained Sophia's letter."** The former headmaster lives in a tower north of Chartreux. All 4 (Chiffon included) go.
3. **f502458:** schoolyard 230110: classmates + Chiffon "we're forgetting something important" (the key).
   - Then 225130 → 225120 → 225110 → Tonkiness 210110 (f507891) → zone 1 (12800) → 260110 → 260120 → 260130 → 260140 → 260150 → zone 2 (54400 ↑) → 260160 → 260170 (f514244) → **Barness 270110 (f515594)**.
   - Arrival scene: Chiffon remembers "weturn the key too the village mayor".
4. **f518038:** Mayor's house 270170 (270110 zone 7, x 92800). The Mayor is on the UPPER floor.
   - Route: walk left to the stair top (lx 6400, ly 36000) → `RC*14` → walk right to lx ≈ 57000 → X.
   - Key returned → the "treasure" is the once-a-decade wheat ("If you know a baker…") → [Ultimate Wheat]. Control back at f520637.
5. **f521838:** Weathervane Inn 270200 (270120 door 211200–216000): Rest 6 CP → "Would you like to save?" Yes → **slot 3** (f523180). Endsave `ea94dbe1a2f3`.
6. **f524221–537426:** 270110 zone 1 (377600) → 260170 → 260160 → 260150 → zone 3 (x 0) → 260180 → 260190 → zone 3 (x 0) → 265170 → 265160 → 265150 → 265140 → 265130 → 265120 → 265110 → zone 1 ↑ (19200) → **Chartreux 300120 (f537426)**.
7. **f537808:** bakery 300270 (300120 zone 5, x 220800). The baker is the man at the far right (about lx 63000), not the woman at the counter.
   - Wheat handed over → he bakes on the spot → **"You obtained Ultimate Bread."** Control back at f540345.
8. **f541160–543781:** item store 300250 (x 102400), clerk about lx 57000. Sales and purchases are under Purchases below.
9. **f544289–546689:** Dancing Piglet Inn 300240 (x 48000). The innkeeper is the old lady behind the counter at about lx 71500 (the man by the fireplace and the old man are NPCs).
   - Rest 8 CP → save offer → **slot 5** → "Game saved." Endsave `704e90e24d19`.

## Party at the end (f546689)
- **Arche** Lv14, 266/266 HP, 45/45 MP. Org Slayer, Scale Armor, Leather Cape, Kitty Ears. Blessed Earring is in the bag.
- **Sana** Lv13, 191 HP, 216 MP. Rune Staff, Scale Armor, Chest Sash, Green Smock, Anti-Poison Ring.
- **Stella** Lv13, 222 HP, 183 MP. Lady's Dagger, Scale Armor, Woven Gloves, Salamander Scale.
- None of the three changed level or equipment during the leg.
- **Gold:** 9104 CP.
- **Items:** Recall Lamp ×1 (new), Revival Drink ×3, Magic Candy ×5, Better Herb ×11, Best Herb, Venom-Away S ×2.
- **Story items:** Sophia's Letter, Ultimate Bread. The Ultimate Wheat went to the baker.
- **The Blessed Earring is not on yet.** The task suggests putting it on Arche before the harpy fields north of Chartreux; I never reached them, so that is for the next leg.

## Purchases / upgrades
- **Bought:** Better Herb ×4 (72), Revival Drink ×1 (350), **Recall Lamp** (500), Magic Candy ×2 (240).
- **Inventory was FULL.**
  - In the shop, buying Revival Drink answered "You cannot carry any more items."
  - The first Better Herb run bought only 4 of 5 for the same reason.
- **Sold old, superseded gear** to make room: Iron Staff 175, Mahogany Staff 1450, Rondel 525, Leather Armor 280, Thick Leather 475 (+2905 CP). That freed slots and raised the gold to 10202 before the purchases.
- **Skipped:**
  - Mind Chocolate: not sold at 300250 (page 2 = Venom-Away S, Holy Water, Magic Candy, Recall Lamp only).
  - The weapons store 300220 (in 300110, x 220800): skipped because of the frame budget.
    - The Day 10 stock put nothing useful in reach: Iron Armor 9800 (Arche) is 700 short at 9104 CP, and Bastard Sword is 10500.
    - Bronze Gauntlets 2800 would replace Arche's Leather Cape; I did not compare them.
    - The next leg passes the store (300110) on the way to the north exit (left of the candy store, x 80000). It is the place to buy Iron Armor once gold reaches about 10.8k (9800 plus ~1000 reserve).

## Game facts / walkthrough corrections (EN-old)
- **School, Day 11:**
  - The staff room is **230140's upper floor**, not a separate room. Sophia stands there at about lx 62000.
  - 230130 upper floor: reach it by walking up the left stairs to the stair top (9600, 32800), then `RC*14` right. Walking right from the stair top goes back down. The right side is analogous.
  - 230150 = nurse (ground left, Ms. Remi, rest + save), library (ground right), a teacher's prep room (upper left) and a 2nd classroom (upper right).
- **Ultimate Wheat:**
  - It comes **after** you return the key. Chiffon's reminder fires on arriving in Barness 270110.
  - The Mayor (270170) is upstairs: from the stair top at the left (6400, 36000) use `RC*14`.
- **Bakery 300270:** the baker = the man on the right (about lx 63000). He bakes the bread on the spot (no waiting, no day change).
- **265160 rose bush (x 202800–211200, 11 × 70400):**
  - It is solid again on this trip and blocks the road to Chartreux. Stella leading, fire mode, `UX`/`DX`/`LX` burns it; a search took 553 f.
  - `ent` swaps the leader Arche → Sana → Stella. The bush resets per visit.
- **260160 (Barness side):**
  - The top-left exit is reached from the top platform (ly ≈ 24600, x 52000–100000) with a dash-jump left onto the cliff (x < 38400). A full jump from the low pocket under it falls short.
  - A two-stage search works: platform first, then `L . L*6 LC*30`.
- **Exit zones held by foes:**
  - 260180's x 0 exit and 265170's x 0 exit were held by a Mothbee / Blue Slime.
  - An exit search with `score = -lx` parks at the zone and never fires it. A search scoring the summed hostile HP kills the holders and then exits.
- **Inventory cap:** the shop refuses with "You cannot carry any more items." Selling superseded gear is the way out (the sell list asks Yes/No; default Yes).
- **Nurse Ms. Remi (230150):** "Rest / Buy Medicine", then "I can save your progress, too" (Yes/No). It is a free heal + save in the school.

## Named state / helpers (scratchpad `/tmp/claude-1000/-opt-src-OpenSummoners/0553de8c-f334-4cbb-a0d1-f526e0ceeca7/scratchpad/`)
- `door.sh RUN X`: walk to x (±300), press ↑, wait for control, print the zones.
  - A dash overshoots, and ↑ at 1.6k away from a door does nothing: approach within ~300.
- `srch.sh RUN TARGET L|R [TL]`: exit search with a directional score + `--apply-best` (`HPMIN` env).
- `fight.sh RUN TARGET [TL]`: same, scored by −(sum of HP of targetable faction-2 actors in slots 0–9). Use it when foes hold an exit.
- `step.sh`, `actors.sh` (from d10e).

## Tool gaps / notes
- **Budget:** the Barness detour (school → Tonkiness → Barness → back east → Chartreux) is ~13 field rooms each way. Day 11 steps 1–3.2 do not fit in 40k frames.
  - The next task should start at the Chartreux inn (`save:704e90e24d19` or `--from story-d11a`) and take 3.0 onward.
- **Gates take ~30 min each on bunker** at ~546k frames: check 1991 s, verify 1661 s, initial replay 1647 s. As the movie grows, gating at two thirds roughly doubles the cost.
- An exit-search score of plain `-lx` parks at a zone that foes hold. A built-in "zone-hold-aware" goal or score in `rd search` would help, for example `zrest == 0` plus the hostile-HP term.
- `rd text` does not show the shop's "You cannot carry any more items" box contents in a way `step.sh` could catch. A structured shop / Yes-No / message-box state would help.
- I put a shell `timeout` on one `step.sh` call (local bunker session, no Windows ssh involved) before remembering the card's rule. No harm, but noting it.

## Repo edits
None. No commits.

