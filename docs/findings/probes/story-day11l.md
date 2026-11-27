# probe story-day11l

run `story-d11l`; accepted 2026-11-27 04:40

check PASS (tools/drive/tasks/story-day11l.md)
verify MATCH (+ real device)
filed tests/scenarios/story-day11l/trace.jsonl (40206 frames)

## driver's report

# story-d11l report

Run `story-d11l` took all three 7.5 chests and the 7.6 [Pitch Black Key], and ends on a save at fountain 2. The fancy doors were not reached: the gargoyle pair in 450220 does not move (details below), and [Celebrity Ring] and [White Iron Ore] are not taken.

- **Frames:** f1 → **f40206**, about 39.8k of play from the load at f360.
- **End position:** room 450225 at the fountain 2 statue (lx 69528, ly 36000). Everyone is at full HP/MP, in control, Arche leading, sword sheathed.
- **End saves:**
  - First fountain save: `58136d59e290` at f24937, slot 14.
  - Final save: **`cf1d4316f966`** at f40206, slot 15 "Labyrith of Night 10:46", all three Lv18. Next leg: `rd new NEXT --from save:cf1d4316f966`.
  - Slots 1–13 and 24 are untouched. Slots 14 and 15 had the shortest play times.
- **Gates:**
  - `rd check`: **PASS** (step 1 at f361, step 2 at f627, no forbid hit).
  - `rd verify`: **MATCH** (frame 40206, rng 9a5c3008, semv b2d9e1183a7ba881, fb 5e8dca290b00ae95).
  - End frame I looked at: `/mnt/ephemeral/opensummoners/drive/runs/story-d11l/view/look_f040206.png`. The session is stopped.

## Route
| frame | what |
|---|---|
| 360 | Loaded slot 13 (Continue = title cursor 1; the load screen cursor starts on the newest file) |
| 1427 | 450220: the 2 bottom Centurions killed |
| 2967 | Up the shaft and stairs to the top corridor; Centurion and Blazemage killed by f4259 |
| 4771–5264 | **Sana's Heal Water II** brought all three to full |
| 5938 | 450220 top-right zone 1 (needed ↑) → 450200 bottom-left |
| 6557–7291 | Bowman and Corporal in the 450200 bottom corridor killed |
| 8614–8882 | **Lift 70202 at x 113600** (a cage on a rail, y 26800↔~93000, about 660 frames per round trip) up to the tower top |
| 10469 | Tower Centurion killed |
| 11032 | Rode stone block 70210 to the chest block; 2 Rattlesnakes 51052 killed |
| 11373 | **[Stronggirl Juice]**, chest (232000, 25600) |
| 13012 | Centurion on block 2 killed |
| 13822 | **[Recall Lamp]**, chest (321600, 32000) |
| 14558 | Cast Unison on the ground under the ledge |
| 15409 | Unison boost onto the ledge (top at y 42400) |
| 16604 | **[Magician's Outfit]**, chest (475200, 44800) |
| 16970 | Stronggirl Juice used on Arche (max HP 288 → 292) |
| 17360–21850 | Walked back: block 12 → chest blocks → tower → lift down → corridor → gate at x 0 |
| 23253 | 450220 → fountain 2. Magician's Outfit equipped on **Stella** (Def 95→107, Res 51→75) |
| 24937 | Drank and saved, slot 14 |
| 28179 | 450220 bottom-left zone 2 (67200, 115200) → **450230** (2 Corporals killed first) |
| 29518 | Top-left corridor, then dropped down the left shaft into the bottom-left area |
| 30–32k | Guards killed: 51241, Blazemage 51242, Centurion. **Arche reached Lv18** |
| 32178 | Unison boost up the 16000 step at x ≈ 163000 |
| 32500 | **[Pitch Black Key]**, chest (97600, 86400) |
| 32706 | Sana was killed (bats and slimes); revived with a Revival Drink at f33165 |
| 35367 | 450230 zone 1 (297600, 57600) ↑ → 450220 bottom-left |
| 37458 | Bottom Centurions killed |
| 38473 | Ice Dragon (51262, 655 HP) killed |
| 38.8k | Gargoyle tries, both failed |
| 39134–40206 | Fountain 2: drank and saved slot 15. **End.** |

## Party at the end
- **Levels:** Arche, Sana and Stella are all **Lv18**.
- **Arche:** 303/303 HP, 55/55 MP, level cap 21. Org Slayer, Iron Armor, Leather Cape, Kitty Ears.
- **Sana:** Green Smock, Scale Armor, Rune Staff, Chest Sash, Anti-Poison Ring.
- **Stella:** Lady's Dagger, **Magician's Outfit**, Woven Gloves, Magic Bracelet.
- **Money:** 23326 CP.
- **Pitch Black Key:** 1 held.

**Healing is better than the brief said.** The item menu shows Better Herb ×3+1+1+1 (two more Better Herb rows were seen before two quick-heals), Best Herb ×3, Revival Drink ×2, Revival Drink EX, Magic Candy ×3, Mind Chocolate, Recall Lamp ×2, and Venom-Away S ×3. Also carried but not equipped: Blessed Earring, Salamander Scale, Orange Beret.

**Skipped:**
- [Celebrity Ring] and [White Iron Ore]. They are in the area past 450230 zone 2, and the frame budget ran out.
- The unequipped accessories. Earlier legs had already passed on them.

## Walkthrough corrections and game facts (EN-old)
- **The 450220 "gargoyles" 70410 at (192000 / 220800, 117600) are background statues.** Arche walks straight through both with the sword sheathed. Pushing gives no push pose and the actor x stays at 220800, and V does nothing. The d11k guess was wrong: step 6's "push right gargoyle left" is most likely in the area after 450230's top-right exit.
- **450220 → 450230** is zone 2, the bottom-left stairs.
- **450230 layout:**
  - Zone 1 (297600, 57600) ↑ leads back to 450220.
  - Zone 2 (432000, 35200) at the top right leads to 450240.
  - The key chest (97600, 86400) is in the bottom-left area. Reach it by dropping down the far-left shaft (x 32000–44000) from the top-left corridor.
  - Coming back from the bottom-middle slime room needs a Unison boost at x ≈ 163000.
- **450220 top-right zone 1** needed ↑ while standing on it; walking in did not fire it.
- **Recall Lamp** in the Labyrinth sends you to **Tonkiness 210110**, not to the fountain. I rolled it back.
- **Sana's Heal Water II** is on numpad `n8`: 6 MP, about 180 frames from press to heal. It heals the member with the lowest HP %, and works on members far away. This makes healing nearly free, since Sana has about 290 MP. Spell slots: n8 Heal Water II, n9 Force Field, n0 Antidote, n5 Splash, n6 Mermaid Robe, n7 Turbid Squall.
- **Auto-guard** (`lstate` 5 while `foe` is 1) blocks V and swallows short moves. Kill or move away from nearby foes before opening chests.
- **450200 pit falls** send you back to a far "last ledge" at (368012, 71200), not to the edge you fell from. Moving planks under 450200 drop you into the pit.
- **New enemy codes:** 51052 Rattlesnake (about 100 HP), 51062 Vile Bat (about 80 HP), 51262 Ice Dragon (655 HP), 51241 (263 HP).
- **Item text and slots:** the chest message spells it "Strongirl Juice". Arche's Stronggirl Juice went into base max HP (+0xa68), raising max HP by 4.
- **Statue saves:** the slot list confirmed both saves (slot 14 = 10:41, slot 15 = 10:46).

## Named state (run-local)
- `m0`–`m2`: party HP. `phmin`: their minimum.
- `e0`–`e15`: HP of hostile character slot N.
- `nfoe`: number of live hostiles.

## Tool gaps
- **"One cell only" misfires.** It shows up whenever the start state is doomed: on a plank about to drop, mid-air in an attack pose, or with auto-guard swallowing input. It reads like an action problem when it isn't.
- **`rd text` misses menu item lists.** The Items page text is not captured, so I had to Read screenshots.
- **`rd map` leaves out the lift and the moving planks.** The 70202 / 70210 objects are not drawn, so routes in 450200 were unclear.
- **Long searches move the shell to the background after 180 s.** I had to wait on an output file each time.

No repo edits and no commits.
