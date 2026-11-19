# probe story-day8h

run `story-d8h`; accepted 2026-11-18 16:54

check PASS (tools/drive/tasks/story-day8h.md)
verify MATCH (+ real device)
filed tests/scenarios/story-day8h/trace.jsonl (131926 frames)

## driver's report

# story-d8h: clover puzzle done (370157) → 370156 → 370155 → 370150 (climbed back to zone 3) → 370160 → 370170 → 370175 → 370180 (stopped at the entry; boss NOT reached)

**Run:** `story-d8h`, created with `RD_HOST=bunker ./rd new story-d8h --from story-d8g`. It starts at f108586 in 370157 with control, a 3-member party (`party` = 7) and Arche leading. It ends at **f131926** in **370180** at the entry (0, 112800). Arche leads with her sword drawn. Everyone is alive and no foe is near. That is about 23.3k frames of movie.
- The movie was cut back by a checkpoint load from f133963: 2k frames of failed 370180 exploration were dropped, so the next leg starts with a healthy party.
- Every rd command used `RD_HOST=bunker`. Nothing ran on the Windows box, and there was no DISK LOW.

**Gates:**
- `rd check` **PASS**: step 1 at f108587, step 2 at f111218.
- `rd verify` **MATCH** at f131926 (rng c620886e, semv e1979c6011839520, fb b4153f656f7fef11).

**End save:** none. No save point was passed, so the last save is still **4ce2d13efdf6** (Korat inn, slot 21). To keep this progress, continue with `rd new NEXT --from story-d8h`. The checkpoint at the end is r180a@131926.

## Route (frames)
- **f108601–109042:** Sana leading cast Heal Water I twice (`n2` is already selected, X×150, ~60 frames to resolve). Sana 74 → 145, Arche 112 → 216.
- **370157 clover puzzle** (walkthrough 7.3, solved by **one member, no helper**):
  - **The block:** 70210 shuttles by itself at 200/frame between x ≈ 54600 and 97600, with its top at y 36000. The lever 370157601 (53200, 34800), operated with V from the ledge, toggles it (`lev` = 1 means stopped).
  - **The vents:** the 3 ceiling vents (x 65.6k, 77.2k, 88k) cycle independently. Each runs 132 frames on and 132 off (period 264), with phases offset by about 60 / 96 frames. A rider under an active vent takes ~13 damage and is knocked into the pit (−10 HP, back to the ledge).
  - **What worked:** stop the block at its left end with the lever (f109884). Then a search (cell includes `frame % 264`, `bx`, `lev`) found: V the lever, hop on the block and ride through the vent gaps to the right ledge. That took 251 frames and reached x 107088 at f110135.
  - **Clover lever 370157511** (136000, 33600) = **V at f110897**: "In the distance, you can hear the sound of machinery activating." `clov` (the lever's state) = 1.
  - The 70810 above it is a clover plate, not a blower.
  - ⚠ The lever only works when you are standing still next to it. Two R presses within ~50 frames trigger a DASH, which overshoots the lever by about 12k.
- **f110981:** switched leader to Sana (on the left side) and walked into the left exit. **The stranded Arche transferred with the room change** (all members go with the leader) → **370156 at f111217**.
- **370156 → 370155 at f111576** by search (outran the Dragon Pup and the Salamandra).
- **370155 → 370150 (entry 4, bottom right) at f113005** by search. It took 1389 frames over the pits and vents.
- **370150, from the bottom back to zone 3 (top left).** This is the hard part, ~11.8k frames.
  - Crossed the bottom chasm west (f113852). Arche pushed the floor block 70210 by the vent 370150511 at (123200, 184000) on the way.
  - **Shaft up** at x 86–106k (y 96k → 160k):
    - Jump-through ledges sit only at y ≈ 171k / 158k / 145.6k.
    - From the top ledge, chained `C*30 R*6` / `C*20 R U X` (an air jumpkick for extra height) reach a ledge on the RIGHT wall at (~107k, 123.6k), at f115296.
    - From there, up and right to the middle corridor (149k, 78k), then left and up the slope into the top region (95k, 47.8k) at f116419.
  - **Followers do not climb the shaft:** AI members stay at the bottom.
    - Arche alone could not clear zone 3's foes: 2 Dragon Pups 51261, 2 **Myconids 51231 (poison)**. She was poisoned twice and the sword rarely hits pups.
    - Undone; then Stella climbed as leader (f118425–119937; the same shaft works with `C*30 L*6` chains and `C*50 L U X`).
    - Her **Phoenix (spell 3: `n3`, X×150; 4 MP; homing)** killed the Myconid and a Dragon Pup at zone 3.
    - Arche AI climbed up behind Stella by itself.
  - **Zone hold from Sana at the bottom:** Sana was 19k from a Salamandra, which held zone 3.
    - Set Follow Mode 'Stay There!' for all three.
    - Leader Sana climbed to y 151k (out of the foe's 20000 y-range), then fell when the menu closed.
    - Fought it as Sana with melee and the water shot (f124301–124793, Sana 30 HP).
    - Then Stella pressed ↑ on zone 3 → **370160 at f124840**.
  - Follow Mode reset to 'Follow Me!' for all at f125768.
- **370160:**
  - Holy Water (party: Arche 216, Sana 145, Stella 182).
  - Venom-Away S → Arche (poison cured).
  - Search across (moving platform over the full-width pit, 2 Dragon Pups) → **370170 at f127836**. Stella reached **Lv10** around here.
- **370170:**
  - Search along the bottom route to the walk-in exit (329600, 76800). Two Salamandras chased and held the zone; Stella's Phoenix and melee killed them (f129985).
  - → **370175 at f130072**.
  - Not taken: chests 370170601 / 370170602 at top right (294400 / 302400, 25600), above vents. Likely the walkthrough's [Guardian Drop] / 3000 CP, but there was no frame budget.
- **370175:** no enemies. Better Herb → Stella (+139) and → Sana (+78). Search across → **370180 at f131818**.
- **Stopped at f131926** at 370180's entry. Exploring further (to f133963: the middle reached at (118.9k, 89.8k), and a failed approach to zone 2 from the left) was cut. It cost ~2k frames and 90 HP with no route found.

## Not done (blockers)
**370180 = the block / lever / vent puzzle of walkthrough 7.4.** Room 96×40 cells.
- **Exit:** zone 2 (35200, 38400) → 370190 (press up). It sits by vent 70700 **370180512** at (54400, 43200), with a 70810 plate at (62400, 48000).
- **Lever 370180511** (code 70216) at (257600, 33600), in the top-right pocket behind plate 70810 (state 1).
- **Blocks:** two 70210 at (192000, 22400) and (216000, 22400), on the top ledge. A floor vent 70700 at (236800, 20800) sits at the pocket's mouth.
- **Chests:** 370180501 at (142400, 38400) and 370180502 at (257600, 64000).
- **Foes:** 4× Salamandra 51043, 2× Myconid 51231, Dragon Pups 51261, 51013.
- **Route facts:**
  - The left column from the entry does NOT reach zone 2 from below: Arche topped out at (40.7k, 89.8k).
  - The middle is reached by going right along the bottom and up, at (118.9k, 89.8k).
  - Expected route (from the walkthrough): up into the top region, push both blocks off the ledge (right one first), push the stack right over the vent at 236800, flip lever 511 (vent 512 off), then zone 2.
- **Then:** BOSS New Age Myconid (370190 → 370200 → 370210), MoH 11, the hot spring and the save are all still ahead.
- **Why I stopped:** the ~40k-frame budget (the run used ~25k frames plus 2k that were cut). Every 370150 fight was slow: Myconid poison, flying pups, and stranded followers holding zones.

## Party at the end (f131926)
- **Arche:** **Lv10**, 147/216 HP, 39/39 MP. Org Slayer, Thick Leather, Leather Cape, Kitty Ears. Sword drawn, leader. No change in gear.
- **Sana:** **Lv10** (up from 9), 149/149 HP, **1/161 MP**. Mahogany Staff, Leather Armor, Chest Sash, Green Smock, Anti-Poison Ring.
- **Stella:** **Lv10** (up from 9), 162/189 HP, 40/132 MP. Lady's Dagger, Woven Gloves, Salamander Scale. Still no body armour.
- **Gold:** 2241 CP.
- **Items:**
  - Left: Better Herb ×2, Hot Springs Bun ×3, Handmade Tart, Sweet Sticks ×5, Revival Drink, **Recall Lamp**, Venom-Away S (3 + 3 doses), Orange Beret, Merrin Card.
  - Used this leg: Magic Candy (on Stella), Holy Water, 3 Better Herbs, 1 Venom-Away dose.
  - ⚠ **There is NO Mind Chocolate** (the story-d8g report and the task listed one). There was also only 1 Holy Water and 1 Magic Candy.
- **Before the boss, the next leg needs MP:** Sana has 1 MP and no MP items are left.

**Upgrades:**
- No shop passed, nothing new found, nothing equipped.
- **Skipped:** the 370170 top-right chests (Guardian Drop / 3000 CP likely) and the 370180 chests 501 / 502, because of the frame budget.

## Game facts / walkthrough corrections
- **The 370157 clover puzzle needs no second member:** stop the block at its left end with the lever, then V the lever from the ledge and hop on as it leaves, timed to the vent gaps.
  - Vent timing: 132 frames on / 132 off.
  - The start/stop lever cannot be reached from the block (V from the block at x 54.5k does nothing).
- **Followers stranded in another part of the room still change rooms with the leader** (370157 → 370156 with Arche on the far side of the pit).
- **The 370150 bottom → top route:** the x 86–106k shaft, chained full jumps, then the right-wall ledge at y 123.6k. AI followers cannot climb it, but Arche AI did follow Stella up once.
- **Zone hold y-range is 20000:** a member just above or below a foe's level does not hold the zone.
- **Myconid 51231 poisons on contact.** A poisoned Arche keeps losing HP (the purple-bubble icon).
- **Stella's spell bar** = 4 slots (Fire Wall, Chaser, Phoenix = slot 3, Disintegrate).
  - Phoenix hits ground foes well.
  - Vs Dragon Pups hovering out of reach it mostly misses: 32 MP for 78 damage in 370160 (that try was undone).
- **F1 → Items from the field:**
  - A heal item on the LEADER gives a ~170-frame cast bar (Better Herb on Arche in 370150).
  - On a non-leader it is instant.
  - Holy Water heals everyone, with one message per member.
- **The F1 Items cursor remembers its page and row.** One extra X can land on the **Recall Lamp → "Do you wish to teleport someplace safe?"** prompt (it happened once; undone).

## Named state (run vars)
- `p0 p1 p2` / `m0 m1 m2` / `id0..2` / `x0..2 y0..2`: party HP / MP / id / position by party slot. `pmin` is the lowest party HP.
- `bx by lev clov`: 370157's block position, start/stop lever and clover lever.
- `va vb vc la lb lc`: 370157 vent phase (`u16(body+0x58)`: 1 off, 2 on) and steam length (`body+0xa4`, 0 to 19200).
- `ehp e1 e4 e6 e14 q0 q1 w2 w3`: specific foe-slot HPs (0 once not targetable).
- `vd ld`: unused.

## Tool gaps
- **`actors()` is unmapped,** so there is no "nearby hostile HP sum" for fight goals. A hand-written 16-slot sum exceeded the 598-token stop-condition limit; I had to name individual slots.
- **No var for party Follow Mode,** and no `rd` helper to set it. Five menu levels per member (Main → Party Commands → Follow Mode → member → option).
- **`-j 2` searches were refused:** the bunker 5 GB pool was full ("580 MB free").
- **"session rebuilt by replaying"** cost 6–10 min of wall time twice.
- **`rd strip` over 120 frames took ~10 min** (replay).
- **The savestate pool exhausted once** with only 2 named checkpoints live.

## Repo edits
None. No commits.

