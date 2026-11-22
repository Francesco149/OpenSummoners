# probe story-day10a

run `story-d10a`; accepted 2026-11-19 16:24

check PASS (tools/drive/tasks/story-day10a.md)
verify MATCH (+ real device)
filed tests/scenarios/story-day10a/trace.jsonl (373565 frames)

## driver's report

# story-d10a: Day 10, Mayberk Manor → Chartreux shops → White Rose Inn save → Weathervane Tower 2.1 (stopped at the 2.2 torch gate)

**Run:** `story-d10a`, made with `rd new --from story-d9f`. Every rd command used `RD_HOST=bunker`. No Windows box, no DISK LOW.
- **Start:** f337196 in 310120, party 7, the Day 10 morning scene ("Arche, wake uu"). Start state confirmed.
- **End:** f372332 in **410130** (tower, 2nd floor corridor with the torches), control, leader Arche.
- Used 35.1k frames. I stopped near the budget because the torch puzzle (2.2) did not yield (see "Blocker").
- **Gates:**
  - `rd check` **PASS**: step 1 at f338714, step 2 at f341121.
  - `rd verify` **MATCH** at f372332: rng 989145b7, semv a73cb86de0c5aee2, fb 0b9b6f5f84cc7b38.
- **End save:** **`eaee1a401f88`**, f354328. Slot **24** "White Rose Inn", party Lv12, after a rest.
  - Next leg from this save: `rd new NEXT --from save:eaee1a401f88`. Or `--from story-d10a` to continue in the tower.
  - Slot 23 (the Mayberk Manor save from d9f) was left untouched.

## Route (frames)
1. **f337196–338713:** the morning scene in 310120. It ends in control (step 1).
   - Stella's Level-up Info then appears on the first F1: she learned **Inferno** (5 MP; Book A, slot 6).
   - Abilities menu: Arche's **Leaping Slash is already "Use"**. In EN-old, abilities are use/don't-use toggles, not slots, so there was nothing to assign.
2. **Leaving the manor:**
   - 310120 top floor → zone 4 (76800, 22400) → hall 310130 top floor.
   - In the hall: walk left down the far-left stair to (12808, 32800), right along the middle floor to (177196, 58400), then left down to the ground floor.
   - Zone 1 (67200) → 310120 ground → zone 1 (25600, ↑) → **310110** at f341120 (step 2).
   - 310110 zone 1 (118400, ↑) → Chartreux **300130** → walk-in right → 300110.
3. **Weapons store 300220** (f342629–343753). Stock and prices are below.
   - Bought **Scale Armor ×2** (4800 each): one on **Sana** (Leather Armor → Scale Armor, DEF 49 → 77) and one on **Arche** (Thick Leather → Scale Armor, DEF 76 → 98).
   - Buying asks "On who?" (with a stat compare), then "Would you like to buy…? Yes/No", and equips on purchase.
4. **Item store 300250** (f344783–346067): bought **Magic Candy ×3** (360 CP). 842 CP left.
   - Storage box (keeper's right side) holds only an Iron Sword.
   - The keeper (NPC 50141 at x 60200) answered X from lx ≈ 55700. X from his right side and V did nothing.
5. **East road 300120 → 265110 → 265120 → 265130 → 265140 → 265150 → 265160:** each room done with one search (score lx). The searches killed slimes, Merkids and bats on the way.
6. **White Rose Inn 265165** (f353212–354329):
   - Innkeeper 50242 (x 65400): Rest 7 CP → "Did you sleep well? Would you like to save?" → Yes → slot screen (cursor on 23) → **D → slot 24** (empty, saves at once) → "Game saved."
   - `rd endsave` → **eaee1a401f88**.
7. **265160 → 265170 → 260190 → zone 2 (44800, ↑) → 260200:**
   - Crate 70205: Stella cannot push it; **Arche must push, sword sheathed**. Switch the leader with `ent` while standing behind it, then `Z`, then `R` → x 43000 (f357376). Then search up to exit (0, 12800) → 260210.
8. **260210:** do NOT press ↑ at the arrival spot: zone 1 (99200) fires on ↑ and sends you back to 260200.
   - Stand at the door (35200, 116000), idle, then ↑ → "Arche put the Thieves' Key in the lock. It fit the lock perfectly!", followed by a great-detective scene (f360283–361385).
   - ↑ → **410110 Weathervane Tower** (f361386): title card, then a long scene with Colm and Chupil (crush teasing; the boys run ahead). Control at f364910.
9. **Tower 2.1 (f364910–371561):**
   - 410110 zone 3 (25600, 19200, ↑) → 410120, ground floor right.
   - Killed 2 Young Harpies (51090) near zone 2, then zone 2 (walk-in 227200, 67200) → 410125 ground. **That is a dead end for the climb.**
   - The ledges in 410125 are 29k units above the floor; a full jump is about 15.6k.
   - **The real route:**
     1. Back in 410120: up the stepped mound (~x 125000) → the floating platform (137700–163300, top 54400) → left onto the upper-right floor (to x 134500, stand y ≈ 36000).
     2. Run-jump right to the harpy platform (150500–211300, stand y 23200) → drop right to the wall ledge → walk-in zone 3 (227200, 32000) → **410125 upper-left ledge** (0, 45600), f369805.
     3. In 410125: mid-left platform → upper level (the search went as far right as x 151552, then back) → zone 4 door (80000, 22400, ↑).
     4. The zone was held only by a Rattlesnake (10 HP left). The perched Harpy 51031 sitting on the door (80000, 20000) did NOT hold the zone. → **410130** at f371561.
10. **410130 (2.2):**
    - 8 torches 70251 on the floor at x 110400–161600 (handles 410130501–508). Their state is 0 = lit, 1 = unlit; slots 2 and 6 are unlit.
    - Gate 70230 (131800, handle 410130511) blocks the corridor. Door zone 2 (35200, 28800) → 410140, beyond the gate.
    - The boys (50120 ×2, 50110) stand at the left. Harpies: 51090 ×3, 51091 ×2.
    - What I tried with Sana leading (see "Blocker"): **Splash (n5)** and **Diamond Dust (n1)**, both cast next to the torch at 161600, and V. A 100 s search over Sana's staff attacks and spells n3/n4/n6/n7 never changed torch slot 7's state.
    - Stopped at f372332, leader switched back to Arche.

## Blocker (2.2 torches)
- **What happened:** "Sana's water puts ALL out" did not happen with Splash or Diamond Dust cast right beside a lit torch.
- **Possible causes I did not test:**
  - It needs a projectile water spell aimed at the torch (Water Blow is not learned yet).
  - It needs a specific position or facing.
  - The torches may be put out by something else (e.g. hitting them with Arche's sword, or Stella lighting the 2 unlit ones first — the walkthrough says not to).
- **Probe for the next driver:** `rd ram mark` → cast → `rd ram diff` on the torch objects. Object state is `s32(s32(world+0x11e0+4*slot)+0xc)`; I defined `t7` for slot 7 (x 161600).
- **Room map:** /mnt/ephemeral/opensummoners/drive/runs/story-d10a/view/map_410130_f372332.png

## Party at the end (f372332)
- **Arche** Lv12, 217/255 HP, 41 MP.
  - Gear: Org Slayer, **Scale Armor** (new, DEF 98), Leather Cape, Kitty Ears.
  - Leaping Slash on (default "Use").
  - Sana's Force Field buffed Arche (+46 ATK) during the 410120 fight; it is temporary.
- **Sana** Lv12, 164/173 HP, ~173 MP.
  - Gear: Mahogany Staff, **Scale Armor** (new, DEF 77), Chest Sash, Green Smock, Anti-Poison Ring.
- **Stella** Lv12, 130/218 HP, 156 MP.
  - Gear: Lady's Dagger, Scale Armor, Woven Gloves, Salamander Scale.
  - Learned **Inferno** (Book A slot 6).
- **1349 CP** (10802 − 9600 armour − 360 candy, + drops).
- **Items:** Magic Candy ×3 (new), Better Herb ×7 (the d9f report said 1; the shop shows "You have 7"), Revival Drink, Venom-Away S ×2, Recall Lamp, Ghost Marshmallow ×3, Sweet Sticks ×2, Doll Hand, plus the Thieves' Key (used on the tower door).
- **Upgrades taken:** Scale Armor ×2 gave +28 DEF for Sana and +22 for Arche for 9600 CP. That beat Iron Armor (Arche only, +32 DEF for 9800).
- **Skipped:**
  - Bronze Gauntlets 2800 (not enough money after the armour).
  - Bastard Sword 10500, Battle Sword 18000, Magic Dagger 9700 (too expensive; Org Slayer kept).
  - Tower 1F side chests (70220 + 70245 at 410120 (16000, 48000) and 410125 (211200, 51200)): these are the walkthrough's spike traps.

## Shop lists seen (EN-old, Day 10)
- **Weapons 300220:**
  - Page 1: Shortsword 2800, Longsword 5500, Iron Staff 350, Mahogany Staff 2900, Rondel 1050, Thick Leather 950, Bronze Armor 2600, Bastard Sword 10500.
  - Page 2: Battle Sword 18000, Magic Dagger 9700, Scale Armor 4800, Iron Armor 9800, Bronze Gauntlets 2800.
- **Items 300250:**
  - Page 1: Herb 10, Better Herb 18, Venom-Away 7, Revival Drink 350, Letter Opener 180, Cloth Armor 280, Honey 24, Chest Sash 78.
  - Page 2: Venom-Away S 40, Holy Water 65, Magic Candy 120, Recall Lamp 500.
- **White Rose Inn 265165:** rest 7 CP; the Rest / Don't Rest / Save menu is followed by a save offer.

## Named state (this run's vars.json)
- `p0 p1 p2` / `m0 m1 m2` party HP / MP, and `pmin`. These are the same definitions as in d9f.
- `cx`: x of the 260200 crate (actor slot 0, `s32(s32(s32(world+0x11e0)+0x40)+4)`). In general, an object's x is `s32(s32(obj)+0x40)+4`.
- `h0..h5`: actor-slot HPs (redefined per room). `t7`: torch object slot 7 state in 410130.
- Helper `/mnt/ephemeral/opensummoners/drive/agents/story-d10a/actors.sh RUN N` lists slot / code / HP / x / y / faction. It is fixed from d9f's version: the rd evaluator's `a and b` returns a bool, so use `b if a else 0`.

## Game facts / walkthrough corrections (EN-old)
- **Day 10 morning:**
  - Leaping Slash needs no assigning (default Use).
  - Stella learns Inferno at Lv12; the Level-up Info shows on the first F1.
- **Manor exit:** 310120 → hall 310130 → 310120 ground → 310110 → 300130 (Chartreux).
- **The weapons keeper sells armour with an "On who?" + stat compare + Yes/No, and equips on purchase.** Item shops buy on `ent` at once.
- **260200 crate:** only Arche pushes it (Stella cannot).
- **260210:** ↑ at the arrival point (zone 1) leaves the room.
- **Tower 1F:**
  - 410110 → 410120 (left) or 410125 (right) both land on ground floors.
  - The way up is in **410120's right half** (mound → floating platform → upper-right floor → harpy platform → zone 3) into **410125's upper-left ledge** → up → the zone 4 door → 410130.
  - The 410125 ground floor cannot be climbed (its ledges are ~29k up).
  - "Climb ledges from the left, then the right ledge → stairs" fits this from 410125's upper side.
- **Harpies:** 51090 Young Harpy (~200–230 HP), 51031 Harpy (210–260), 51091 (258–278, a harpy variant in 410130). Rattlesnake 51052 (~90–105). Kobold Ace seen in 410120.
  - No grab or drop happened during this leg.
  - A perched Harpy did not hold a zone.
- **Sana casting:** select with `n#`, then hold X for ~110–130 frames and release.
  - Heal Water from the leader seat healed **Sana herself** (+85).
  - When she was not the leader, her AI Heal Water and Force Field went on Arche.
  - Splash (n5) = 20126, a radial water burst, 2 MP. Diamond Dust (n1) = 20101, 3 MP.
- 410130 has 8 floor torches (6 lit, 2 unlit) and gate 70230 at x 131800.

## Tool gaps / notes
- **`search -j 2` spawned a second bunker session and ran at 136 ticks/s** (1388 s for a 120 s limit). `-j 1` ran at 500–800 ticks/s. I used `-j 1` afterwards.
- **Search output is cut off by `| cut`.** I lost one best path that way. Write the full output to a file before trimming it.
- **The Bash tool moves foreground calls over ~3 min to the background**, so every search and the ~20 min replays (`new`, `check`, `verify`) needed wait loops.
- **Score design was the main cost in the tower.** A greedy `lx` or `-ly` score pulls the search into dead ends: the 410125 floor, and the far right of the upper level. Waypoint goals plus bounding avoids (`lx < 100000`) worked.
- **`actors(…)` is not mapped in rd** (ACTOR_BASE is None), so I could not write a "no hostile near" predicate as a single program; I used per-slot `hN` names instead.
- **An "object state changed" probe** (e.g. `rd zones --all --diff`) would have made the torch test quicker.

## Repo edits
None. No commits.

