# probe story-day10b

run `story-d10b`; accepted 2026-11-22 11:08

check PASS (tools/drive/tasks/story-day10b.md)
verify MATCH (+ real device)
filed tests/scenarios/story-day10b/trace.jsonl (407920 frames)

## driver's report

# story-d10b: Weathervane Tower 2.2 → 2.6 (Rune Staff, Revival Drink, Water of Life, 2 levers + 2 switches; blocked at the 2.6 tower puzzle)

**Run:** `story-d10b` (from `story-d10a`). Every rd command used `RD_HOST=bunker`. No Windows box, no DISK LOW.
- **Driver history:** a previous driver did f373565–390041 and ran out of quota without writing a report. I resumed the run. Its bunker session had died; `rd status` rebuilt it by replaying 390041 frames in 1368 s.
- **Start:** f373565 in 410130, the torch room (all torches out, gate open).
- **End:** **f407920 in 410215**, control, leader Arche, sword drawn. That is 34.4k frames used.
- **Gates:**
  - `rd check` **PASS**: step 1 at f373566, step 2 at f374143.
  - `rd verify` **MATCH** at f407920: rng 53ac4378, semv 052432058928bbd6, fb 084268a005058b5f.
- **End saves:** **none this leg.** There was no save point before the blocker. The last save is still d10a's `eaee1a401f88` (White Rose Inn, slot 24).
- **Stopped at a real blocker:** the 2.6 wheeled-tower puzzle in 410215 (see "Blocker").
  - Room map: `/mnt/ephemeral/opensummoners/drive/runs/story-d10b/view/map_410215_f407920.png`

## Route (frames)
### Previous driver (from its transcript)
1. **f373565:** leader back to Arche. Through the torch gate → **410140** (the big hall); fought harpies → **410150** (2.3, moving blocks over spikes).
2. **"You found Rune Staff"** at f380623 (410150, x 178140). Checkpoint "equipped" at f381476.
   - **Rune Staff is equipped on Sana.** Evidence: the Items list at f381434 shows the Mahogany Staff with no "E:" tag, and Sana's max MP went from 173 to 208.
3. **Back to 410140:**
   - Rode the platform up (f385341–385464), jumped right to the upper floor (f385524).
   - Sheathed and pushed the block right to bkx 211200, under the lever (f385995).
   - Killed a Rattlesnake (f386412).
   - Stood on the block and pulled the lever with V: lever=1, gate=1 at f386568.
4. **→ 410160 at f386914.**
   - Searches toward the far-right chest 70220 at (164800, 67200).
   - It was opened just before f390041; "You found Revival Drink" was on screen at f390131.

### This driver
5. **410160 Water of Life zigzag** (f390195–392394). The route:
   - from the right floor (stand ly 64800), run-jump left to platform B (x 51200–102400, stand ly 52000), at f390376;
   - jump left to the left ledge (x ≤ 38400, ly 42400), at f390523;
   - jump right to platform A (x 51200–102400, ly 29600), at f390657;
   - jump right to platform D (x 115200–134400, ly 23200);
   - drop right onto the upper floor (x ≥ 144000, ly 29600), at f390890.
   - 70245 = floor spike traps sit on B and D. The search passed them without damage.
   - Two Young Harpies were guarding the chest. Killed them, faced the chest (it opens only when facing it), V → **"You found Water of Life"** at f392240.
6. **Heals (f392394–393356):**
   - Arche used `F` (Quick-heal cast bar, ~140 frames), 58 → 255.
   - Stella was down to 2–15 HP fighting a harpy on the floor below. She died twice in trials; those were reloaded and are not in the movie.
   - A search with `ent` / `F` / attack actions kept her alive and killed the harpy. Stella ended at 204.
7. **→ 410170** (f395050), through the left ledge's zone 2 (0, 32000).
   - A Harpy 51031 on the ledge held the zone; killed it.
   - **The zone fired only on ↑, although `rd zones` lists it as "walk in"** (see tool gaps).
8. **410170, walkthrough 2.5 (f395050–399742):**
   - **Moving blocks over spikes:** 70210 slots 2 and 3 slide along x 225000–284000 at y 41600. Crossed right → left with a search (cell includes the block x), at f395929.
   - **Two-block stack:**
     1. Climbed to the right upper ledge (x 192000–208000, stand ly 13600) at f396562.
     2. Sheathed (Z) and pushed block 70210 (slot 1, bx 195200) left. It fell to the floor at bx 185600 (f396841).
     3. Went down, jumped over it, and pushed it left to **bx 176000**, exactly under the left ledge's right edge (f397419).
     4. Climbed onto the left ledge (x 160000–176000), left of block slot 0 (bx 166400), at f398270.
     5. Pushed that block right off the edge. It landed on the first block: (176000, 28800) on top of (176000, 35200), at f398475.
     6. Pushed the stack left to bx 139200 (f399076). Two stacked blocks DO move together.
     7. Jumped on top and pressed V at the wall lever (139200, 22400): **lever=1, gate 70230 open at f399222**.
   - Left → zone 2 (22400, ↑) → **410180** at f399742.
9. **410180:** switch 1, the wall lever 70216 at (174400, 46400), reached from the floor. A search pulled it (V) at **f400402**; gate 70230 at 179800 opens.
   - Ceiling spike trap 70430 mid-hall.
   - A Harpy 51091 held the exit zone (Sana within 48000 of it); killed it.
   - Zone 2 (252800, ↑; listed as "walk in") → **410185** at f401859.
10. **410185:** switch 2, 70216 at (123200, 46400), pulled at **f402426**. Gate 70230 at 128600 opens.
    - Spike traps 70245 (floor) and 70246 (ceiling) at x 26400–48800 and 148000.
    - Three harpies (51090 ×2, 51031) brought Sana down to 24 HP. A search (`ent` / `F` / Sana's `LX`) got her back to 173; then a fight search cleared all three.
    - Zone 2 (195200, ↑) → **410190** at f403526.
11. **410190, "drop off right edge → hidden stairs down":**
    - Walking right off the arrival ledge (259200, 77600) drops into a pit shaft at x ≥ 281600. Falling straight down is a pit (−13 HP, back on the ledge).
    - **Steer left while falling** (search) to land in a hidden chamber (x 243400–281800, floor ly 141600), at f404077.
    - The chamber's door is zone 2 (252800, 144000, ↑) → 410215. The ↑ fired only with lx ≈ 255700, inside the arch, not at the zone's x 252800.
    - → **410215** at f404847.
12. **410215 (2.6):**
    - Killed Harpy 51091, Young Harpy 51090 and Kobold Ace 51031 (f406568); Arche and Sana leveled up.
    - Then tried the tower puzzle (Blocker). The tower resets when you leave and re-enter, so I went 410215 → 410190 chamber → 410215 (f407605–407746) to restore it.
    - Ended at f407920.

## Blocker: 410215 wheeled scaffolding tower (walkthrough 2.6 "Arche pushes platform right; Sana/Stella stays on it; push back → ledge → star lever")
- **Layout:**
  - The **star lever** 70216 is at (12800, 19200), in a chamber on top of the left ledge. The ledge spans x 0–38400, y 25600–41600, top 25600. Object 70810 is at (12800, 12800).
  - The **tower** 70480 is at b0x 51600 (its left edge), about 9000 wide, standing on the floor (y 64000). Its top platform, object 100144, is at y 38400.
  - Slope "steps" are at (126400, 46400) and (126400, 56000), next to the right door.
  - The left door, zone 2 (19200, 60800) → 410210, is under the ledge, **behind the tower**.
- **What I found:**
  - From the right (the only side reachable from the hidden chamber), Arche can only push the tower **left**. It stops at b0x 38400, touching the ledge.
  - Arche cannot walk through it, with the sword drawn or sheathed.
  - A 240 s search over jumps and air attacks (`R U X` chains) from the floor reached ly ≈ 40000 at best. The tower top needs ly ≈ 32800, so it is not reachable from the floor.
  - Getting someone on top seems to need the tower pushed **right** to the steps (x ~117000–126000), and that needs Arche on its left side.
  - **The hidden chamber in 410190 is a dead end** except for 410215. Its left wall (x 224000–243400) is solid floor to ceiling, and the shaft is a pit that sends you back to the chamber.
- **Hypotheses for the next driver:**
  - (a) The intended start is from **410210** (reached from 410190's lower area, zone 3 (211200, 144000)), entering 410215 through its left door behind the tower. The hidden-stairs route we took would then be a different branch. But from 410215 / the chamber there is no way back to 410190's main area that I found.
  - (b) A member can get on the tower another way: e.g. use Follow Mode "Stay There!" to park Sana / Stella on the step at 126400 while the tower is pushed left past … (unclear).
  - (c) Probe the ruins' wall tops. They look walkable in the picture but are not in the collision grid.
  - Try `rd ram mark` / `diff` while pushing, and look at 100144 (a 100000-range code: maybe a rider / lift).
- **Picture:** `/mnt/ephemeral/opensummoners/drive/runs/story-d10b/view/look_f407920.png` (the ruins); the tower is in `look_f406729.png` and `look_f405083.png`.

## Party at the end (f407920)
- **Arche:** Lv13, 262/262 HP, 41/45 MP. Gear unchanged: Org Slayer, Scale Armor, Leather Cape, Kitty Ears.
- **Sana:** leveled up at f406568 (Lv13 probable), 116 HP at the end. Gear: **Rune Staff** (replaces the Mahogany Staff, which is now in the bag), Scale Armor, Chest Sash, Green Smock, Anti-Poison Ring.
- **Stella:** Lv12 or 13, about 98 HP at the end, gear unchanged.
- **Gold:** 2598 CP.
- **New items:** Rune Staff (equipped), Revival Drink (+1, so 2 now), Water of Life (a Help Out item).
- **Herbs:** Better Herbs / Herbs were consumed by `F` quick-heals. The HUD counter beside the item icon read ×13; I did not audit the exact counts.
- **Skipped:**
  - Stronggirl Juice, the fountain, MoH 14: not reached (blocker).
  - Chest 70220 at 410190 (196800, 144000): it is in the area behind zone 3, which I did not reach.
  - No shops were passed.

## Named state (run vars.json)
- **From the previous driver:** p0–p2 / m0–m2 / pmin; h0–h6 = character-slot HP (slot = index into world+0x1160); chest, bx1/by1, bx2/by2, bkx, lever, gate, c3, c10 = object slots at world+0x11e0+4k (state +0xc, x / y via +0x40 → +4 / +8).
- **Added this leg:**
  - `b3x` / `b3y` / `b2y`: moving blocks in 410170.
  - `b0x` / `b0y`: object slot 0 x / y; the 410215 tower.
  - `tx`: object slot 1 x.
  - `lev` / `gt` (object slots 5 / 4): the 410170 lever / gate.
  - `g2`: object slot 2 state.
  - `gty`, `h3`.
  - All of these are generic object-slot readers; the room decides what each one means.
- **Helper:** `/mnt/ephemeral/opensummoners/drive/agents/story-d10a/actors.sh RUN N` lists the character slots.

## Game facts / walkthrough corrections (EN-old)
- **410160:**
  - Revival Drink chest at (164800, 67200), on the right floor.
  - Water of Life chest at (169600, 32000), on the upper floor, reached by the zigzag described above.
  - 70410 is a gargoyle statue (decoration). 70245 / 70246 are floor / ceiling spike traps.
- **Chests open with V only when facing them**, and not with a foe on top of you.
- **The 410170 block stack (2.5):** "push right one to floor, left of the left ledge's edge" works as: the right ledge's block goes off its left edge to the floor, then gets pushed left to bx 176000 (exactly under the LEFT ledge's RIGHT edge). The left ledge's block is then pushed RIGHT off onto it.
  - A block falls when it is entirely past the edge: bx = edge going right, bx = edge − 6400 going left. Blocks are 6400 × 6400; object x / y = top-left.
  - **A stack moves as one when you push the bottom block.** The lever is on the wall at (139200, 22400); stand on the stack and press V.
- **2.5 switches:** switch 1 is in 410180, switch 2 in 410185. Both are wall levers reachable from the floor (V).
- **Exits:** 410170 → 410180 → 410185 → 410190 are all ↑ doors. `rd zones` called some of them "walk in", but they fired only on ↑.
- **"Drop off right edge → hidden stairs":** in 410190, drop right off the arrival ledge and steer LEFT while falling into the chamber. Straight down is a pit.
- **The leader has inertia:** after release Arche slides ~10k units (~35 frames), and the first ~10 frames of a hold barely move her. Taps under ~12 frames can do nothing.
- **Levers / switches** = 70216, gates = 70230.
- **Harpies:** 51031 here showed up as Harpy / Kobold Ace (the code is reused per room). Harpies hold exit zones even while flying.
- **`F` (quick-heal):** starts a Quick-heal cast bar on the current leader (~140 frames; Arche healed fully).
  - Switching leader with `ent` is refused for a while after a dialogue or while busy. Repeat `ent` with `--until lid == …`.
  - Followers (Stella, Sana) get ganged by harpies when they lag behind. Watch p1 / p2, not just the leader.
- **Arche and Sana leveled to Lv13** in this leg.

## Tool gaps / notes
- **`rd zones` kind label is wrong for these rooms:** zones listed as "walk in" (410160 z2, 410180 z2) fired only on ↑.
  - `+0x274` holds the zone id (1, 2, 3, 4), not its kind.
  - The ↑ trigger x does not equal the zone x: the chamber stairs at 252800 fired at lx ≈ 255700.
- **A search's found path is not always shortened:** one `--apply` search to 410190 applied an **8806-frame** path (refine did not shorten it). I undid it and walked there in ~400 frames.
  - Drivers: check the `found … N frames` line before keeping a path. Maybe `--apply` should refuse paths far above the refine target.
- **The savestate pool ran out** after ~20 checkpoints; `rd ck drop` freed it. Re-attaching the dead session took 1368 s of replay; verify took 1645 s.
- **`rd map` shows only class-10 collision.** The ruins' tops that look walkable in 410215 are not solid. I could not tell from the map whether the tower top (object 100144) is a platform.

## Repo edits
None.

