# probe story-day10c

run `story-d10c`; accepted 2026-11-23 18:35

check PASS (tools/drive/tasks/story-day10c.md)
verify MATCH (+ real device)
filed tests/scenarios/story-day10c/trace.jsonl (417707 frames)

## driver's report

# story-d10c: Weathervane Tower 2.6 (wheeled-tower puzzle solved → star lever → Stronggirl Juice → MoH 14 → fountain save)

**Run:** `story-d10c`, started with `rd new story-d10c --from story-d10b`, which replays the d10b movie: f407920, room 410215. Every rd command used `RD_HOST=bunker`. There was no DISK LOW.
- **Note:** `--from end:story-day10b` is the wrong start. It boots from the scenario's last slot save, which is d10a's White Rose Inn (frame 1). I recreated the run with `--force` from the run movie.
- **End:** **f417707 in 410210** (the fountain room), in control, leader Arche, sword sheathed. That is **9787 frames** past the start.
- **Gates:**
  - `rd check` **PASS**: step 1 at f407921, step 2 (left 410215) at f413274.
  - `rd verify` **MATCH** at f417707: rng 0bedd2eb, semv 13ccbb54cceb2705, fb 3e928a8238bf24d5.
- **End save:** **`4e700cca7925`** (`rd new NEXT --from save:4e700cca7925`). It was written at the fountain in 410210 into **slot 1**: all 24 slots were full, so I overwrote the oldest one (slot 1, "Town of Tonkiness, Arche LV1"). Slot 24, the White Rose Inn save, is kept.

## Blocker solution (410215 wheeled tower)
d10b's premise was wrong: Arche **can** pass the tower.
- The tower base 70480 is only **8800 × 4000** (from `device_init.c`: SETUP 8800,4000). Its deck 100144 (8800 × 800, at y 38400) is a separate carried object. The lattice between them has no collision.
- A plain run-jump left over the base (`L*90 LC*14 L*30` from lx 87208) put Arche on its left side at f408054.

The sequence:
1. **Push right:** Z (sheathe), then hold R. The tower travels from b0x 51600 to **b0x 112000**, the right end of its range, at f408874.
2. **The steps by the right door:** class-1 one-way slope cells at **x 124800–128000**, tops y 54400 and 44800. Stand lx are 48800 and 39200.
   - Jump with `C*30` from the floor at lx ≈ 126500: lower step, then `C*30` again: upper step.
   - From the upper step, `LC*20 . R*16` lands on the deck (stand ly 32800, lx ≈ 117700). The `R*16` brakes; without it the leader slides off.
3. **Followers do NOT climb up after the leader**, so a member has to be put on the deck by hand:
   - Switch to **Sana** (`ent`), climb the steps the same way, and land her on the deck at f410759.
   - Press **F3 twice**: "Stay There!" (HUD F3 icon = figure + anchor). F3 cycles through 3 modes; a third press restores Follow.
4. Switch back to Arche. She stands on the upper step.
   - `DC*4` (down + jump) **drops through the class-1 step**. Do it twice to reach the floor. Walking off the step instead carries her over the base to the wrong side.
   - Hold L with the sword sheathed. The tower and Sana on the deck ride left together to **b0x 38400**, flush with the ledge (f411822). Riders are carried (`entity_carry_riders`).
5. Switch to Sana. `LC*31` from the deck lands on the ledge top (stand ly 20000) at f411904.
   - Walk to x ≈ 12900 and press V on the **star lever** 70216 (slot 5): state 1 at f412094. Message: "In the distance, you can hear the sound of machinery activating."
6. Sana jumps right off the ledge (`RC*20 R*30`), which takes her over the tower to the floor. F3 once restores Follow, then switch back to Arche (f412729).
   - Jump the base (now at 38400) to the left and press ↑ at the left door (lx ≈ 21000). Arrived in **410210** at f413273.

## Route after the lever (frames)
- **410210** arrival at the UPPER level (38400, 20000). The "stairs up" go into this room.
  - Chest 70220 at (27200, 22400), V while facing it: **"You found Strongirl Juice"** at ~f413540 (the EN-old spelling).
  - Used it from F1 → Items (page 5, last row) → Use → On who? → **Sana: "Sana's Max HP increased by 4"** (187 → 191) at ~f414230.
  - F1 first showed the pending Level-up Info pages (Arche 13, Sana 13). **Sana learned Antidote.**
- **Drop:** walked left off the upper platform into the left shaft, landed at the bottom (y 64800), then walked right.
- **70100 at (113600, 56835) is MoH 14**, picked up by walking through it: "You obtained a Mark of Heroism!" at ~f414900. Then the Congratulations scene; script ended at f415755 with HP and MP restored.
- **The fountain** is the angel statue on the raised dais at the LEFT of the lower corridor, around x 24000–31000.
  - V at **lx 26400** gives "Water flows from the fountain. Would you like to drink the water?" Yes → "Your health is restored!" → "Would you like to save?" Yes → slot list.
  - Saved to slot 1 at ~f417650.
  - The identical statue 70410 at x 124800 (right end) is decoration only: V and ↑ do nothing.
- Stopped at f417707. Walkthrough order in-game: MoH 14 comes before the fountain if you walk right first.

## Party at the end (f417707)
- **Arche** Lv13, 262/262 HP, 45/45 MP. Org Slayer, Scale Armor, Leather Cape, Kitty Ears (unchanged).
- **Sana** Lv13, 191/191 HP (+4 from the Stronggirl Juice), 216 max MP. Rune Staff, Scale Armor, Chest Sash, Green Smock, Anti-Poison Ring. New ability: Antidote (not assigned / not checked).
- **Stella** Lv12, 218/218 HP, 156 max MP. Lady's Dagger, Scale Armor, Woven Gloves, Salamander Scale.
- **Gold:** 2598 CP.
- **Notable items:** Revival Drink ×2, Water of Life, Recall Lamp, Magic Candy ×3, Ghost Marshmallow ×2, Better Herbs (many), Venom-Away S ×3, Chicken Leg, Sweet Sticks, Merrin Card, Doll Hand.
- **Not taken / skipped:** the walkthrough's next optional items: Door → [Mystery Juice]; left door → [Best Herb]; right door → [Blessed Earring]. The task said stop at MoH 14 / fountain. No shop was passed, so no gear upgrade. I did not open the Abilities menu for Sana's Antidote.

## Named state (run-local, `rd ram story-d10c list`)
- `o{k}x` / `o{k}y` / `o{k}c` (k = 0..3): object slot k x / y / code (`s32(world+0x11e0+4k)`).
- `lev5`: object slot 5 state. In 410215 that is the star lever.
- Character slot y: `s32(s32(s32(world+0x1160+4*i)+0x40)+8)`. Sana was slot 4 in 410215.

## Game facts / walkthrough corrections (EN-old)
- **410215 tower:**
  - Base 70480 = 8800 × 4000, jumpable from the floor. Travel range is b0x 38400..112000.
  - The deck 100144 rides at y 38400 (stand ly 32800) and carries riders.
  - The steps at x 124800–128000 are **class-1 one-way slopes**: you jump up through them and land only when falling, and `D`+`C` drops through.
  - Followers never climb to the deck: put a member there as leader, then "Stay There!" (F3 ×2).
- **Walkthrough 2.6 as played:** jump over the tower base; push it right to its end; the member goes up via the steps onto the deck; Stay There; Arche pushes the tower left to the ledge; the member jumps to the ledge and pulls the star lever.
- **The left door of 410215 leads to 410210's UPPER level.** "Down, stairs up" = through this door. The juice chest is right there.
- In 410210, **MoH 14 is in the lower corridor at x 113600**, and the **fountain = the angel statue at x ≈ 26400** (V). The fountain gives a heal prompt, then a save prompt.
- The F3 Follow-mode cycle has 3 states; HUD icon: two figures + arrow = Follow, figure + anchor = Stay There.
- All 24 save slots are now in use. Later drivers must overwrite one (the prompt defaults to Yes).

## Tool gaps / notes
- **`--from end:SCENARIO` starts from the scenario's last slot save, not from its end state.** For story-day10b that is the inn save. The task header's "start: story-d10b" needs `--from story-d10b` (the run movie). Cost: one wasted ~21 min replay.
- **Hold `--goal` searches with `--hold N`.** Goals that are true mid-jump (ly thresholds) were applied while the leader was still sliding off; `--hold 30` fixed it.
- **The leader's inertia makes `{toward()}` overshoot by 2–5k.** Brake with a short opposite press.
- **`rd text` does not show the menu item list or the sub-menus** (Items, Use / Discard, slot list). I had to read them with `rd look`.
- The bunker rebuild, check and verify each replay ~418k frames, ~21–23 min apiece.

## Repo edits
None.

