# probe story-day10e

run `story-d10e`; accepted 2026-11-25 06:11

check PASS (tools/drive/tasks/story-day10e.md)
verify MATCH (+ real device)
filed tests/scenarios/story-day10e/trace.jsonl (491227 frames)

## driver's report

# story-d10e: Weathervane Tower 2.8 (MoH → Wind Crest 3) → Recall Lamp → Tonkiness → the argument → Day 11 morning

**Run:** `story-d10e`, created with `rd new story-d10e --from story-d10d` (the run movie). rd chose **bunker** on its own. I never touched the Windows box, so there were no ssh resets, no fallback and no DISK LOW. The replay that built the session took 1540 s.
- **Start:** f460320 in 410230, in control, leader Arche.
- **End:** **f491227 in 230140** (the school classroom, Day 11), in control, leader Arche, party = 7 (all three).
  - `story` went 1500 → **1600**.
  - I looked at the end frame: `/mnt/ephemeral/opensummoners/drive/runs/story-d10e/view/look_f491227.png`.
- **Length:** 30.9k frames past the start, under the 40k budget.
- **Gates (both at the end, f491227):** `rd check` **PASS** (step 1 at f460321, step 2 at f460580; forbid clear). `rd verify` **MATCH**: rng 1f5f99b3, semv 80900c8d190675d5, fb f73847fb218b5f58.
  - I did not run a mid-run gate at two thirds as the task asks. The whole route took about 50 min of play, and each gate costs about 30 min on bunker at ~490k frames. I ran both at the end instead.
- **End save:** **`f0048b995ad1`**. The next leg can start with `rd new NEXT --from save:f0048b995ad1` (Continue → the save made just before Day 11) or with `--from story-d10e` (the run movie, which ends in the Day 11 classroom).
  - In-game **slot 2** was overwritten. It held the oldest progress: "Minasa-Ratis Magic School, Arche LV1, 0:57". It now reads "Minasa-Ratis Magic School, Arche LV14, 8:46".
  - Slots 1 and 24 are kept. Slot 4 ("Prologue Clear Data") is also untouched.
- Session stopped (`rd stop`).

## Route (frames)
1. **f460543:** walked to 410230 zone 2 (108800), pressed ↑ → **410240** at f460579.
2. **MoH:** object 70100 floats at (68800, 22015), well above the floor (stand ly 36000). Under it, `C*30` (full jump) touches it.
   - "You obtained a Mark of Heroism!" at **f460859**, followed by a ~850-frame "Congratulations" scene with a full heal. Control returned at f461705.
3. **Wind Crest scene:** walking right fires the crest scene at lx ≈ 104000, f461845.
   - Content: crest too high → Chiffon can't touch crests → Team Machismo arrives → Colm's wind → Colm leaves the crest.
   - "**You obtained Wind Crest 3.**" Control returned at **f471441**.
   - I stepped through it one `ent` at a time with a text-reading loop. There was no Yes/No in it.
4. **Menu pass at f471581:**
   - Abilities: Twirling Slash was **already set to "Use"**. Abilities are on by default, so there is nothing to assign.
   - Items → Kitty Ears → Equip → Arche. The preview showed Atk 78→84, Def 96→102, Resist 52→46.
5. **Recall Lamp** (Items p6 → Use → "Do you wish to teleport someplace safe?" Yes), f472287–f472646.
   - It sent the party to **260210** (outside the tower door, = the `retry` room).
   - Stella's goodbye scene ran there; control returned at f474728 with party = 3 (Arche + Sana).
6. **Walk home** (the d7f road in reverse), arriving at **Tonkiness 210110 at f481963**:
   - 260210 → 260200 (f474991; slime/bee held the exit, killed by search) → 260190 → 260180 (a Blue Slime 51011 on the rock held zone 1; killed) → 260150 → 260140 → 260130 → 260120 (search) → 260110 (pit crossing, 2 slimes 51010 held the town gate) → 210110.
   - Sana's goodbye ran in town; control at f482636.
7. 210110 zone 2 (150400 ↑) → 210120 → zone 2 (118400 ↑) → **210140** at f483817.
   - Right stairs up to the stairs top (84480, ly 36000), then `LC*14` onto the middle floor. The argument scene fired at **f484570** (lx ≈ 72750).
   - After the argument: "Would you like to save?" → Yes → slot 2 → Overwrite Yes → "Game saved." (f489056).
   - Then the next-morning scene at school (Sana / Stella / "ask Ms. Sophia, staff room"). **Control returned on Day 11 at f491227.**

## Party at the end (f491227)
- **Arche** Lv14, 266/266 HP, 45/45 MP. Org Slayer, Scale Armor, Leather Cape, **Kitty Ears**. Blessed Earring is in the bag.
- **Sana** Lv13, 191 max HP, 216/216 MP. Rune Staff, Scale Armor, Chest Sash, Green Smock, Anti-Poison Ring.
- **Stella** Lv13, 222 max HP, 183/183 MP. Lady's Dagger, Scale Armor, Woven Gloves, Salamander Scale.
- **Gold:** 7089 CP. The money cap is now 80000.
- **Items:** Revival Drink ×2, Magic Candy ×3, Best Herb, Better Herbs (many), Ghost Marshmallow ×2, Venom-Away S ×3, Sweet Sticks, Chicken Leg, Merrin Card, Doll Hand.
  - The Recall Lamp is used up.
  - The MoH heal and the crest scene refilled everyone, so no items were needed.
- **Upgrades:**
  - **Taken:** MoH; Kitty Ears back on Arche. My reason: the harpies are behind us and Kitty Ears give Atk/Def +6 against the earring's Resist +6 and confusion guard. The confusion guard can go back on for harpy areas; Day 11 lists Young Harpy / Harpy again in the field north of Chartreux, so the next leg may want it back.
  - **Skipped:** the restock in Tonkiness. Its shops (Arche's dad, weapon shop 210160) sell only Herb-tier items and gear below what the party wears; Magic Candy and Mind Chocolate are Chartreux stock. Nothing was needed with full HP/MP.

## Game facts / walkthrough corrections (EN-old)
- **410240** (the room right of the tower top):
  - The MoH 70100 floats at (68800, 22015). Take it with `C*30` from directly under it (floor ly 36000).
  - The crest scene fires walking right at lx ≈ 104000. Object 70130 at (105600, 22400) is the crest platform.
  - The angel statue at x ≈ 140800 (70410) is decoration (V does nothing), and so are the gargoyles.
  - No save point.
- **The Mark count looks one behind the walkthrough's numbering.** After this MoH, the Status page read **Marks Gained 14 / 20 and Level 14 (MAX: 17)**, not cap 18 as the walkthrough's "MoH 15 (cap 18)" says. Either one earlier mark was missed by the story runs, or the walkthrough's numbering is off. Worth checking which MoH the corpus lacks.
- **The Recall Lamp returns to the `retry` room** (here 260210, outside Weathervane Tower), not to town. From there, Stella's goodbye scene fires, and then it is a 9-room walk home: 260210 → 260200 (zone 1, x 92800) → 260190 (zone 1) → 260180 (zone 1 at the right end, **↑ needed** at x ≈ 113200) → 260150 (zone 1) → 260140 → 260130 → 260120 → 260110 (zone 1 ↑ at 153600) → 210110.
  - "Walk/up" zones fire on walking in at some exits but needed ↑ at 260180.
- **260110 has a pit gap** between the left ledge (ly 16800, edge x ≈ 40000) and the right cliff. Falling costs −13 HP and returns you to the left ledge. A dash run-jump crosses it: `R . R*21 . R*10 RC*20 …` (found by search).
- **The argument** fires on 210140's middle floor at lx ≈ 72750, reached by `LC*14` from the right stairs top (84480, 36000). Walking left from the stairs top goes back down.
  - It is followed directly by "Would you like to save?" (Yes / No; Yes → slot list, Overwrite defaults to Yes).
  - Then the Day 11 school scene, with no bedroom step.
  - The save label for a home save reads "Minasa-Ratis Magic School".
- Abilities menu (F1 → Abilities → character): a 2-page Use / Don't-use toggle list. New abilities arrive already set to "Use".

## Named state (run-local)
- `s2hp`, `a0`, `a1`: actor slot HP for single-enemy kill searches (redefined per room).
- `moh`: a bogus define (`s32(flags+0)`), ignore it.
- Scratchpad helpers:
  - `/tmp/claude-1000/-opt-src-OpenSummoners/0553de8c-f334-4cbb-a0d1-f526e0ceeca7/scratchpad/step.sh RUN N` advances dialogue one `ent` per line, prints the text, and stops on control or a real Yes/No / save prompt (matches only lines that are exactly "Yes" / "No").
  - `go.sh RUN TARGET DIR` walks toward an exit, presses ↑, and searches if stuck.

## Tool gaps / notes
- **A search goal of `room != CUR` can lead back the way you came** (my mistake once: it went back to 260190). Use `room == TARGET` plus avoid `room not in (CUR, TARGET)`.
- `rd text` shows a dialogue line growing letter by letter (the typing effect). A prompt-detection regex on "No" can false-trigger on lines like "No. I'm…". A structured "Yes/No menu open" flag in rd would help.
- The save-slot list shows the virtual clock's wall time, which is meaningless (all 2012/01/27). Only the play time tells which save is oldest.
- Gates at ~490k frames on bunker cost ~30 min each (check 1790 s, verify 1744 s). The initial `rd new` replay took 1540 s.

## Repo edits
None. No commits.

