# probe story-day9e

run `story-d9e`; accepted 2026-11-19 14:09

check PASS (tools/drive/tasks/story-day9e.md)
verify MATCH (+ real device)
filed tests/scenarios/story-day9e/trace.jsonl (293977 frames)

## driver's report

# story-d9e: 400160 tombstones → secret door → 400180 (spike-trap Crypt Key, left gate) — partial 4.6

**Run:** `story-d9e`, made with `rd new --from story-d9d`. Every rd command used `RD_HOST=bunker`; no Windows box; no DISK LOW.
- **Start:** f282638 in 400160, control, party 7 (Arche, Sana, Stella; start state confirmed).
- **End:** f293977 in **400180** at (213980, 39200), the upper-right corridor past the left gate. Control; Arche leads, sword drawn.
- Used 11.3k of the 40k frame budget. I stopped at the budget mark, not at a blocker. Most of the time went on Ghost Mage fights and a slow bunker.
- **Gates:** `rd check` **PASS**: step 1 at f282639, step 2 at f290406. `rd verify` **MATCH**: rng 48df40fe, semv bd87d492ae67374e.
  - verify first printed "SESSION drifted" (frame and RNG equal, **0 differing dwords**). The evidence is in `runs/story-d9e/verify-drift-f293977`. The clean replay matched.
- **End saves:** none (no save point on this route). The last save is still `a2a16253c9bb` (White Rose Inn, slot 22). The next leg must use `--from story-d9e`.

## Route (frames)
- **400160 → the tombstone corridor (f282638–285376).**
  - Drop down the shaft at x ≈ 93000. The bottom corridor (y 93600) has **two pits**: x 115000–128000 and 140800–153600, with a block at 128000–140800 (top ly 96800) between them.
    - Do two separate dash-jumps: `RC*12` at lx ≈ 112000 lands on the block, then `RC*25` at lx ≈ 138500 clears the second pit.
    - One long jump falls into the second pit.
    - The ledge (y 68000, x 96600–172000) is a dead end at x 164000.
  - Go right along the lower corridor (y 80800) to its right end (295600, 55200).
  - **`LC*67` (hold the jump about 67 f)** climbs the step-overhang into the upper corridor (found by search). `LC*30` is not enough.
- **Tombstones (f285376–289796).**
  - The pushable tombstones are **70411** with handles 400160511 / 512, at x 200900 / 218500.
  - **70800** (192000 / 225600) are the **targets**: they go to state 1 when a tombstone reaches them.
  - Steps:
    1. Stand between the two tombstones with the sword sheathed.
    2. Push 512 right until it stops at 226500 (f~288086).
    3. Push 511 left until it stops at 194500 (f~288600; I pushed it by accident during a fight).
  - Both 70800 → state 1 and **zone 5 (70440, handle 400160503) moved from (208000, 54400) to (208000, 35200)**: the secret doorway between the tombstones.
- **Two Ghost Mages (51082) in 400160 cost most of the leg.**
  - They heal themselves to full when low (75 → 214 and 34 → 194).
  - They bind the leader (green swirl, ~40 f with no movement).
  - They hit for 40–100. One got Sana to 4 HP; one KO'd Stella (I rolled both back).
  - **Kill recipe:** Stella `n3` Phoenix (hold X 160, ~4 MP, ~76 + 22 damage per cast, homing) twice, then the AI finishes. Mage 1 died at f287752, mage 2 at f290260.
- **f290405 → 400180** (zone 5, ↑ at lx 207708).
  - Menu: Magic Candy ×2 on Sana (+52, +56 MP).
  - The spike trap is **70430 at (110000, y 12800)**, a crusher in the shaft x 108800–124800.
    - It falls (12800 → 60800 in ~45 f) when the leader comes within ~8000 of it (lx ≈ 102000). Crush damage is ~45–98.
    - It sits at the bottom ~25 f, then rises at ~200 units/f.
  - **Ride it:** stop at lx ≈ 104500, wait until `spy == 60800`, then `R*4 RC*10 R*6` puts you on top (ly = spy − 5600). Ride to the top (ly 7500) and walk left.
  - **Chest 70220 (94400, 16000) = [Crypt Key]** (f~291868).
    - It opened only with V standing at lx 90400 facing right (from lx 89680 it did nothing).
    - The message box closed with `C` (`ent` taps did not close it).
  - Keyhole **70217 (139200, 56000)**: V → "You used Crypt Key. The Crypt Key has broken." Gate 70230 (149400) opens (f~292340).
  - A Skeletal Soldier (51027, 253 HP) that chased Sana was killed by Arche (f293309, +51 CP).
  - Past the trap again (jump over it while it sits low, f293530). Stairs up-right to the upper-right corridor (y 39200) → end.

## Where the next leg starts (400180, f293977)
- **Ahead:**
  - A floor spike platform 70430 at (238000, 24600) and a Skeletal Soldier pile (51027 at 225600).
  - Ghost Mage 51082 at (270000, 27200) and a Ghost at (265200, 35200).
  - Gate 70230 (handle 400180505) at **335000** with keyhole 70217 (324800, 35200). **No Crypt Key in hand.**
  - Behind that gate: zone 2 (358400, 35200, ↑) → 400190 and zone 3 (406400, walk-in) → 400185.
  - Upper room x 217000–272000, y 9600–25600, with 70800 (246400, 12800, handle 400180512) — unexplored.
- **Behind:** Ghost Mage 51082 (slot 5, 142 HP) roams the trap area.
- **Open question:** the walkthrough's "right door needs another key → left door (fall into skeletons) → left pit hugging right → [Crypt Key], [Guardian Drop]".
  - In EN-old the trap key had to go into the **left gate (149400)**, because that gate is the only way from the trap chamber to the right half.
  - So the "left door / fall into skeletons / left pit" is probably reached from the upper-right part: the 70430 spike platform at 238000 (a trapdoor?) or the upper room.
  - Next: `rd look` at x 230000–250000, and `rd zones --all` after stepping on 70430.

## Party at the end (f293977)
- **Arche** Lv11, 144/225 HP, 39/39 MP: Org Slayer, Thick Leather, Leather Cape, Kitty Ears.
- **Sana** Lv11, 109/158 HP, ~79/176 MP: Mahogany Staff, Leather Armor, Chest Sash, Green Smock, Anti-Poison Ring.
- **Stella** Lv11, 52/195 HP, ~32/140 MP: Lady's Dagger, Scale Armor, Woven Gloves, Salamander Scale.
- No level-ups or gear changes. **8810 CP** (+431).
- **Items:** Magic Candy ×2 left (2 used), Ghost Marshmallow ×3 (200–260 HP each, not ×1), Better Herb, Venom-Away S ×3, Revival Drink, Sweet Sticks ×2, Recall Lamp, Merrin Card, Doll Hand.
- **Crypt Key:** 0 held (1 found, 1 used).
- **Upgrades:** none available (no shop or gear chest). The Doll Hand is still unequipped / unknown.

## Game facts / walkthrough corrections (EN-old)
- **400160 tombstones:**
  - The pushables are 70411 (handles 511/512); the 70800 are target plates (state 1 when a tombstone reaches them).
  - Arche is blocked by the tombstones themselves (the "pillars").
  - The secret door is zone 5 (70440), which moves to (208000, 35200).
  - The tombstones end at 194500 / 226500.
- **400160 navigation:**
  - The bottom-corridor pits sit at x 115–128k and 140.8–153.6k.
  - The upper corridor is reached from the lower corridor's right end with a long held jump (`LC*67`).
- **Ghost Mage (51082):**
  - Heals itself to near full once it is low.
  - Binds the leader for ~40 f.
  - Hits for 40–100 and drives followers' HP down fast.
  - Phoenix (Stella slot 3, ~4 MP) is the efficient answer. A dash during the fights skids ~11000 units after release.
- **400180 spike trap:** 70430 is a falling ceiling crusher triggered by proximity. Ride it up to chest [Crypt Key]. Walking under it while it sits low or rises is safe.
- **Party switching:** `ent` from Arche goes Arche → Sana → Stella. A press sometimes does not register even 22–26 f apart; check `lid` after each press.
- **Menu:** F1 → Items opens on the last page. Pages hold 8 rows; ←/→ change page. "Use" → "On who?" (Arche, Sana, Stella).

## Named state (this run's vars.json)
- Copied from d9d: `p0 p1 p2 pmin nfoe fhp sk f3 gm f2 pl pr`.
- 400160:
  - `gm4`, `gm11`: Ghost Mage HP, actor slots 4 / 11.
  - `mx my`: mage position (redefined per target).
  - `t1x t2x`: 70800 x.
  - `g1x g2x`: tombstone x (object slots 14 / 12).
- 400180:
  - `spx spy`: spike trap, object slot 9.
  - `gm5`: mage HP, actor slot 5.
  - `sk0`, `sx`: skeleton slot 0 HP / x.
- Helper (scratchpad): `actors.sh RUN N` lists actor slots as code / hp / x / y / targetable.

## Tool gaps / notes
- **Bunker was slow for searches: 44–450 ticks/s, against ~1400 elsewhere.** The bunker swapped (4.4 GB swap, ~25 % iowait) and the rhost process held ~900 MB. `play` was fine. I avoided searches where I could.
- **`rd rewind` replays from boot (~3–15 min)** even with a checkpoint 100 frames earlier. Use `ck load` or `undo` instead.
- **`rd ck drop` works now:** it frees the savestate and keeps the session.
- **Long `{…}` inputs hit the server's 598-token limit** (`mx`, `lx` and the like expand to long chains). A lightweight alias or a pre-resolved pointer would help.
- **`rd verify` reported "SESSION drifted" with 0 differing dwords**, then matched on the clean replay. Evidence: `runs/story-d9e/verify-drift-f293977`.

## Repo edits
None. No commits.

