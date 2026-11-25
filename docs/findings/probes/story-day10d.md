# probe story-day10d

run `story-d10d`; accepted 2026-11-24 11:15

check PASS (tools/drive/tasks/story-day10d.md)
verify MATCH (+ real device)
filed tests/scenarios/story-day10d/trace.jsonl (460320 frames)

## driver's report

# story-d10d: Weathervane Tower 2.6 chests → 2.7 levers ×3 → BOSS Wyvern (killed)

**Run:** `story-d10d`, created with `rd new story-d10d --from story-d10c` (the run movie). rd chose the session host itself and picked **bunker**. I never used the Windows box, so there were no ssh resets, no fallback, and no DISK LOW.
- **Start:** f417707 in 410210, in control, as the task describes.
- **End:** **f460320 in 410230** (the tower top), in control, leader Arche. The Wyvern is dead and its corpse is on screen. I looked at the final frame: `runs/story-d10d/view/look_f460320.png`.
- **Length:** 42.6k frames past the start, about 2.6k over the ~40k budget. The boss died at f459230 and I finished the fight rather than stop with it at 1364 HP.
- **Gates:**
  - Mid-run, at f448722 (the boss intro ended): `rd check` **PASS** and `rd verify` **MATCH** (rng f98d770a, semv c34f2df6044fb9fb).
  - Final, at f460320: `rd check` **PASS** (step 1 at f417708, step 2 at f418075), with the forbid clear. `rd verify` **MATCH**: rng 18fb30e7, semv 8ff7cea939e78f96, fb f48334c8f5153c57.
- **End save:** **`4e700cca7925`**, unchanged from d10c. No statue or fountain is on this route, so there is no new save. The next leg should start with `--from story-d10d` (the run movie). A `save:` start would go back to the 410210 fountain.

## Route (frames)
1. **410210 zone 2** (↑ at x 99200; this is the "Door") → **410190** bottom chamber, f418108.
   - Harpies attacked at once. Stella Phoenix searches killed 3 of them.
   - Chest 70220 at (196800, 144000): **[Mystery Juice]** at f419173.
2. **410190 zone 5** (54400, 124800; the "left door", reached up the stairs on the bottom left) → **410200** entry 2, f421791.
   - Killed a Kobold Ace and harpies.
   - Floor → central shaft at x ≈ 128000. There are three class-1 platforms at stand ly 71200 / 58400 / 45600; `C*30` from each reaches the next.
   - From the top platform, `LC*30 L*30` lands on the upper-left corridor (ly 36000).
   - Past the floor spike trap 70245 (x 100000) to chest 70220 at (59200, 38400): **[Best Herb]** at f427081.
   - Used the Mystery Juice on **Stella** (max MP 179 → 183) at f427853. Sana's Heal Water topped up the party.
3. **[Blessed Earring]**, the chest 70220 at (216000, 67200), sits on a tall block on the room's right edge. It cannot be jumped from the floor (≈19000 up).
   - Route: shaft platforms → `RC*30 R*30` onto the right corridor → walk right up the stairs to the top area (ly 26400; zone 1 → 410190 is here).
   - Then go right, **walk off the room's right edge (x > 230560), and steer LEFT while falling**. You land on the block (stand ly 64800); straight down is a pit outside the tower.
   - Opened at f430146. I equipped it on **Arche**, replacing Kitty Ears.
4. Back the long way: down, shaft, top area, then **410200 zone 1** (198400, 28800) → 410190 entry 4 (top right) at f432915.
   - Went left along the top. Phoenix and Arche cleared 2 Kobold Aces and a Harpy.
   - The star wall 70230 at x 101400 is open (state 1). **Zone 6** (70400, 28800) → **410220** at f435262.
5. **410220, levers ×3** (lever 70216 / gate 70230 pairs, by slot):

   | Lever | Lever position | Gate | Pulled at | How to reach it |
   |---|---|---|---|---|
   | 1 | (131200, 140800) | 138200 | f436991 | Bottom floor. |
   | 2 | (73600, 83200) | 67800 | first f439157, again f440906 (it was off again by f440216, cause unknown — see facts) | The stairs right of gate 1 go up-right to a junction (181000, 116000). The up-left stair band starts at ly 102200 there; a search found the jump. Walk left down the band to the mid floor (ly 84000). |
   | 3 | (123200, 28800) | 128600 | f443135 | Through gate 2: stairs up-left to the platform (ly 58400). `RC*42`-type jump to the landing (49308, 40800). Up-right stairs to the top (ly 29600). |

   - **Dead end / fall off the edge:** walk right past gate 3 to the room's right edge (x 202800, the top level ly 20000). Fall and **steer LEFT** to land at (180000, 71200), the doorway of **zone 2** (176000, 73600) → **410225**, f443687.
6. **410225 ("stairs up, no monsters")**:
   - Left along the floor → stairs → landing (28808, 109600).
   - `C*14 RC*16 R*10` up → right to the landing end (x ≈ 68000, ly 84000).
   - `C*14 RC*16 R*10 L*8 RC*30 R*8 C*11` up the right side → top-right corridor (129188, 42400).
   - `L*16 .*16 L . L*12 LC*20 L*10` dash-jumps to the upper-left ledge (100060, 32800). Walk left to **zone 2** (38400) → **410230**, f445687.
7. **410230 = the tower top. The boss is here, with no statue or fountain before it.**
   - Scene (Team Machismo "stuck part-way", Chiffon spots it, Stella: "Arche, aim for the head!"). The fight started at f448722.
   - **Wyvern killed at f459230.** Control returned at f460207, then I viewed the Level-up page (Arche Lv14).

## Boss facts (EN-old, Easy)
- **Wyvern: code 100179, actor slot 4, 5759 max HP.**
  - Named `bh` / `bx` / `by` = slot 4's HP / x / y in this run's vars.
  - It flies in from the right (x 70160, y 13100). Then it fights on the ground with its head to the left, and its body spans about 60k of the arena.
- **Killed with Arche (sword) score searches:** `--score "-bh + pmin/2"`, `--cell "(lx // 6400, bh // 40)"`, actions X / R.X / L.X / UX / `C*12 X` / X+UX, `--avoid pmin < 80`.
  - It took 9 rounds of 400–1800 frames each, about 1.4 HP per frame including the AI's damage. Hits on the head do 60–70.
  - One mid-fight heal round used Sana as leader, searching for `n2` Heal Water casts.
- The boss drops 2830 CP (gold 4228 → 7058). No item was observed.
- The fight drained MP: at the end Sana had 12 MP and Stella 29.

## Party at the end (f460320)
- **Arche** Lv14 (new), 186/266 HP, 45/45 MP. Org Slayer, Scale Armor, Leather Cape, **Blessed Earring** (Kitty Ears is now in the bag). New ability: **Twirling Slash** (Lv14), not assigned.
- **Sana** Lv13, 121/191 HP, 12/216 MP. Rune Staff, Scale Armor, Chest Sash, Green Smock, Anti-Poison Ring.
- **Stella** Lv13 (she levelled 12 → 13 in 410190), 222/222 HP, 29/183 MP (+4 from the Mystery Juice). Lady's Dagger, Scale Armor, Woven Gloves, Salamander Scale.
- **Gold:** 7058 CP.
- **Items:** + Best Herb. Mystery Juice used. Revival Drink, Magic Candy ×3, Ghost Marshmallow ×2, Better Herbs (×3 + ×1 + …), Chicken Leg, Recall Lamp and the rest are unused.
- **Upgrades:**
  - **Taken:** all three optional chests (Mystery Juice → Stella, Best Herb, Blessed Earring → Arche).
  - **Trade-off of the earring:** the equip preview showed Kitty Ears = Attack +6, Defense +6; the Blessed Earring = Resist +6 plus confusion protection ("makes it very hard for foes to confuse you"). I did not swap Kitty Ears back for the boss, to save menu frames. The next leg can swap back if harpies are gone.
  - **Skipped:** no shop on the route. Twirling Slash is not assigned (the Abilities menu was not touched).

## Game facts / walkthrough corrections (EN-old)
- **2.6 optional chests are all in 410190 / 410200, reached from 410210 zone 2** (the door at x 99200):
  - The "Door → [Mystery Juice]" chest is right beside the arrival point in 410190 (196800, 144000).
  - The "left door" = 410190 zone 5 → 410200, whose upper-left corridor holds [Best Herb] past a floor spike trap.
  - The "right door" = 410200's top area (410190 zone 4 ↔ 410200 zone 1). "Drop off edge" = walk off 410200's RIGHT room edge and steer left onto the block with [Blessed Earring].
- **410200 shaft (x ≈ 128000):** class-1 platforms at stand ly 71200 / 58400 / 45600, each reached with `C*30`.
  - Left corridor: `LC*30` from the top platform (ly 36000).
  - Right corridor: `RC*30 R*30`.
- **2.7, the star wall:** 410190's 70230 at x 101400 (state 1 after the d10c star lever). It is on the TOP level of 410190, left of the 70810 at 105600. The stairs beyond = **410190 zone 6 (70400, 28800) → 410220**.
- **410220 levers** (70216 → gate 70230): bottom (131200) → mid (73600) → top (123200).
  - **Lever 2 went back to state 0 at some point after I pulled it**, during a harpy fight next to it, and I had to pull it again.
  - It was confirmed back at 0 by f440216 (rd zones); the exact frame was not pinned. A hit, an attack or an AI action probably toggles it.
  - V on a lever fails while `foe` == 1 and also from some x offsets. It worked from about the lever x + 500 facing it, standing idle.
- **"Dead end → fall off edge"** = 410220's top level runs right off the room edge at x 202800. Steer LEFT while falling to land in the 410225 doorway at (180000, 71200).
- **"Stairs up (no monsters)"** = 410225. It has three jump-ups (see Route 6). Its zone 2 leads to the tower top.
- **There is no save statue or fountain between the 410210 fountain and the Wyvern.** The walkthrough's "save before the boss" can only mean the 410210 fountain.
- **The BOSS is in 410230 (the tower top), triggered on arrival.** 410230 zone 2 (108800) → 410240 is the next leg's "stairs right → MoH 15".
- Wyvern stats: see Boss facts.

## Named state (run-local, `rd ram story-d10d list`)
- `p0 p1 p2` / `m0 m1 m2`: party HP / MP (Arche / Sana / Stella), and `pmin`.
- `bh bx by`: Wyvern HP / x / y (actor slot 4 in 410230).
- `aN` (N = 0..12): actor slot N's HP (0 when the slot is unused). Redefined per room.
- Helper `/tmp/claude-1000/-opt-src-OpenSummoners/0553de8c-f334-4cbb-a0d1-f526e0ceeca7/scratchpad/actors.sh RUN N` lists actor slots: code, hp/max, x, y, targetable.

## Tool gaps / notes
- **Searches with a single `-ly` or `-lx` score wander when the route doubles back.** Three applied `--apply-best` paths cost 3–10k frames (410220 lever 3: 4k frames to nowhere; 410225: a 3010-frame path that hand play did in about 1300). I undid them and played by hand.
  - **Wanted:** `--apply-best` should report the path length before applying it, or refuse paths longer than N.
- **An `undo` sometimes replays from boot** (when the savestate pool is exhausted): about 3 minutes with no warning.
- **`look --at F` replays** (about 6 minutes here). Prefer `ck save` before a probe.
- **`actors()` in rd is not mapped** (ACTOR_BASE None). A sum over 16 slots exceeds the 598-token limit, so a "nearby enemies' HP" stop condition is impossible. Per-slot defines were the workaround.
- **`rd tap ent --until script == 0` presses one more `ent` after control returns, which swaps the leader.** I undid it.
- The first verify rebuilt the session (savestates lost, 26 min). Each check or verify on bunker costs about 25–27 min at 450k frames.
- `rd text` shows no menus (as known). I read the equipment previews with `rd look`.

## Repo edits
None. No commits.

