# probe story-day11c

run `story-d11c`; accepted 2026-11-25 11:06

check PASS (tools/drive/tasks/story-day11c.md)
verify MATCH (+ real device)
filed tests/scenarios/story-day11c/trace.jsonl (639326 frames)

## driver's report

The run reached its goal. The Grand Gourmand took the Ultimate Bread and opened the secret path. The party is in control inside the Archmage's Tower's first room, **440110**, at **f639326**, with leader Arche. Both gates are green, but there is no new save. The harness refused to let me write `/mnt/ephemeral/opensummoners/drive/agents/story-d11c/report.md`, so this message is the full report.

**Run:** `story-d11c`, started with `--from story-d11b`. It is 31.1k frames past the start (f608262).
- **Host:** rd chose bunker (from session.json). I never used the Windows box, so there were no ssh resets, no fallback and no DISK LOW.
- **Bunker session died twice:** around 00:40 and 03:00 CET on 2026-11-25. Each rebuild replayed the whole movie (2426 s, 2220 s) and wiped every checkpoint's savestate. Search speed also swung between 54 and 900 ticks/s.
- I stopped the session with `rd stop`. No repo edits, no commits.

**Gates** (run once each, at the end):
- `rd check`: **PASS**. Step 1 at f608263, step 2 at f611100, forbid clear.
- `rd verify`: **MATCH**. Frame 639326, rng 83e42eb5, semv ed5300ca6469683e, fb 2be01253534bed1e. It took 1950 s on bunker.

**Save:** no new end save. The route passes no inn or goddess statue after the Guardpost inn, and the statue in 440110 is decoration (V does nothing). The last save is still slot 6, `31deb5360576`, at the Mountain Cat Inn. The next leg should use `--from story-d11c`.

## Route (frames)
| frame | what |
|---|---|
| 608262 | start: 430120 inn, in control |
| 608993–609521 | Item shop: the old lady opens with **V** (X does nothing). Read the Magic Bracelet, bought nothing. |
| 609963–610188 | Inn 2F drawer (70363, (65600, 32000)) gave **[Best Herb]**. The 2F closet is empty. |
| 611099 | out to 430110, the Guardpost |
| 611109 onward | Barrel Power Drop: about 2.2k frames of tries failed. I loaded back, so none of it is in the movie. |
| 611698 | 430110 → 420180 |
| 612858–614994 | 420180: climbed onto the top ledge, killed a Saber-panther, zone 2 (top right) → **420220** |
| 615024–617697 | 420220: killed 4 foes, zone 1 (bottom right) → **420200** |
| 617727–620106 | 420200: jumped up the crates on the scaffolding, zone 3 (top left, needs ↑) → **420210** |
| 620533–621353 | 420210 ledge chests: the bag was full, so the items stayed in the chests |
| 621775–623989 | **Grand Gourmand** (talk with V) takes the Ultimate Bread himself; `story` = 1600 |
| 624030–625024 | Menu: used items to free bag slots and heal |
| 625621 / 625931 | Took **[Life Dew]** and **[Guardian Drop]** |
| 626365 | Guardian Drop used on **Sana** |
| 626647 | 420210 zone 3 (↑ at x 23200, between the two rocks) → **420230** |
| 626686–631962 | 420230: fights, then the climb to the top platform, → **420240** |
| 632863 | Healed: Chicken Leg → Stella, 2 Better Herbs → Sana and Arche |
| 634619–637754 | 420240: killed the Trippy Myconid and harpies at the tower door, ↑ at (163200, 92800) → **440110** |
| 637754–639326 | Entrance scene ("monsters in the tower"), then **control at f639326** |

## Purchases and upgrades
- **Magic Bracelet (11000) not bought:**
  - Its preview only says "REALLY bolsters the user's spirit"; the shop shows no numbers.
  - The walkthrough (5.2) puts a free one in the headmaster's closet in this tower.
  - Buying it would have left about 1.7k of 12.7k CP.
- **Shop stock:** page 2 of the shop is only an Orange Beret (75 CP).
- **Guardpost barrel Power Drop: skipped.** It stayed shut after about 2.2k frames of tries (details under corrections).
- **Iron Armor: skipped.** There is no weapons store on this route.

## Party at the end
- **Arche** Lv14, 136/266 HP: Org Slayer, Scale Armor, Leather Cape, Kitty Ears. The Blessed Earring is still in the bag.
- **Sana** Lv14, 128/201 HP: Rune Staff, Scale Armor, Chest Sash, Green Smock, Anti-Poison Ring. Got the Guardian Drop.
- **Stella** Lv14, 161/228 HP: Lady's Dagger, Scale Armor, Woven Gloves, Salamander Scale.
- **Gold:** 14079 CP.
- **Items:** Better Herb about 11, Best Herb ×5, Life Dew, Revival Drink ×3, Magic Candy ×4, Recall Lamp, Venom-Away S ×3.
- **Used this leg:** 2 Ghost Marshmallows, Honey, Chicken Leg, 2 Better Herbs. The Ultimate Bread was given away.

## Walkthrough corrections (EN-old)
- **Room chain:** 430110 → 420180 → **420220** → **420200** → **420210** → **420230** → **420240** → **440110** (tower first room).
  - 420180 zone 2 (top right) is reachable only from the top ledge.
  - 420200's exit to 420210 is zone 3 at the top left (x 0) and needs ↑.
  - 440110's exits: zone 2 ↑ at (150400, 25600) → 440120; zone 1 → 420240.
- **420210 holds the step 3.3 items and the Gourmand:**
  - [Life Dew] chest at (83200, 12800); [Guardian Drop] chest at (75200, 12800).
  - The Grand Gourmand stands at about (27200, 25600) between two rocks.
  - The "quick jumps on wooden crates" are the crates (code 70205) on the 420200 scaffolding.
- **420220 zone 2 → 420210:** it sits on a floating ledge about 32000 above the floor. It is unreachable now, probably a spot for the Unison boost jump.
- **420230 climb:** there is no way up from the ground.
  - Get onto the middle block top (ly ≈ 29600, x 61000–80000).
  - From x 61138, dash-jump right with a jumpkick onto the top platform (x ≥ 96500, ly ≈ 16800): `C*60 R U X .*6 R . R*10 RC*30 R . R*10 RC*60`.
  - Then go right to the exit at x 176000.
- **Guardpost barrel 70302:** nothing opened it.
  - Tried: X and V with the sword sheathed, V with the sword drawn swept over x 62000–84000, and a ↓-stab.
  - `src/script_engine.c` sends 70302 to the plain examine handler `0x407970`. The Power Drop may be in a different barrel, or need a condition.
- **Bag cap is 48 slots (6 pages × 8):**
  - With a full bag, a chest says "You found X inside the treasure chest", keeps the item and closes again (state back to 0).
  - Free slots before looting.
- **Talk with V:** the Guardpost shop lady and the Gourmand respond to V, not X.

## Named state (run-local)
- **`h0` / `h1` / `h2`:** party HP that works in any room: `s32(s32(s32(world+0x2784)+0x4030+4*i)+0xa64)`.
- **`x1` / `y1`, `x2` / `y2`:** follower positions, via the same party-slot chain: `+0x9f4 → +0x40 → +4 / +8`.
- **Enemy HP sums:** `es`, `el`, `eh`, `ep`, `en` and `eb`, each defined per room.

## Tool gaps
- **Bunker sessions die:** twice in about 6 h. Each death costs a ~40-min rebuild and drops all savestates.
- **`rd map` misses scaffold platforms:** the 420200 scaffold beams and crate tops are not drawn, so route planning there was blind.
- **`--apply-best` lands mid-jump:** with a `-ly` score the best cell is often mid-air. Scoring only standing states (`lstate == 0`) would help.
- **Searches slipped out through a back exit:** the room change comes a few frames after `script` turns 1, so `room not in (…)` misses it. Adding `(room == R and script == 1)` to the avoid fixes it.
- **No bag-full signal:** `rd text` shows the "found X" line, not the refusal after it.
