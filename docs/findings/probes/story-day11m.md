# probe story-day11m

run `story-d11m`; accepted 2026-11-27 04:53

check PASS (tools/drive/tasks/story-day11m.md)
verify MATCH (+ real device)
filed tests/scenarios/story-day11m/trace.jsonl (37295 frames)

## driver's report

# story-d11m report

Run `story-d11m` cleared step 7.6 and most of 7.7. It took [Celebrity Ring], [White Iron Ore], [Tough Drop], [Mystery Juice] and [Star Rod], and used the [Pitch Black Key] on the 450250 lock. It did not reach the 4-way junction: the run stopped in room 450260 at f37295 because of the frame budget.

**There is no end save.** No statue or fountain is ahead before fountain 3. Fountain 2 (450225) is five rooms back, across the water room, so I didn't go back. The next leg should start from the run itself: `rd new NEXT --from story-d11m`.

- **Frames:** f1 → **f37295**, about 36.6k of play from the load at f660.
- **End position:** room 450260, bottom right at the opened Star Rod chest (lx 271436, ly 87200). Party at full HP (303/236/265), in control, Arche leading, sword sheathed.
- **Gates:**
  - `rd check`: **PASS** (step 1 at f661, step 2 at f927, no forbid hit).
  - `rd verify`: **MATCH** (frame 37295, rng 486cef97, semv 3471c6707b6a17b9, fb 744669fe3a58cae1).
  - End frame I looked at: `/mnt/ephemeral/opensummoners/drive/runs/story-d11m/view/look_f037295.png`. The session is stopped.

## Route
| frame | what |
|---|---|
| 640 | Title → Continue → load screen, page 2, slot 15 (the load cursor started on slot 7, page 1) |
| 926 | 450225 → 450220 |
| 1608 | 2 Centurions killed at the arched door |
| 1780 | **450220 z3 (204800, 118400) ↑ → 450240** (shortcut: the task route went via 450230) |
| 2622 | 450240 upper Centurion killed |
| 3041 | Unison cast at x ≈ 422000 on the upper floor (ly 42400); boost onto the top-right ledge (ly 20000) |
| 3481 | **[Celebrity Ring]**, chest (433600, 22400) |
| 4235–4744 | Dropped down the shaft at x ≈ 243000 to the bottom floor; went right over the two pits |
| 7396 | Bottom-right guards killed (Centurion, 51242, 51241, slime) |
| 8144 | **Pushed the right gargoyle** from 414400 to 388800 onto plate 70800 (384000). Plate state 1, door zone 70610 state 0: fancy doors appear |
| 9779 | Went back left past the middle room: Ice Dragon + 2 Centurions killed |
| 12103 | Unison boost from the bottom-left room (x ≈ 74000) up into the side chamber (ly 71200) |
| 13023 | **[White Iron Ore]**: skeleton at x ≈ 47000 (V → "Something is shining…" → Yes) |
| 15064 | Back at the fancy doors |
| 15505 | Celebrity Ring equipped on Stella |
| 15631 | 450240 z2 (377600, 89600) ↑ → **450245** (the water / boat room) |
| 16845 | Entry Centurion and the 3 guards riding the boat killed |
| 17116 | Sana cast Mermaid Robe (n6) |
| 17487 | **[Tough Drop]**, underwater chest (91200, 57600), opened by Arche with V |
| 17740–18803 | Boat ride, then a jump onto the right bank |
| 21603 | 2 Centurions and a bat at the exit killed |
| 22941 | Tough Drop used on Arche (Defense +1) |
| 23174 | 450245 z2 (259200, 44800) → **450250** |
| 24800 | **[Mystery Juice]**, chest (276800, 32000) top right |
| 25745–27098 | **Trap at the top-left gate** (48600): Ice Dragon + 2 Blazemages. Won with Unison |
| 27446–28767 | Down the left shaft, along the bottom over the stone blocks, under the falling spikes |
| 30152 | 2 Centurions and 51242 killed |
| 30494 | **Pitch Black Key used** on lock 70217 (281600, 96000). Gate 288600 opens |
| 30678 | z2 (304000, 86400) → **450260** |
| 33523 | Bottom trap section (Centurion + 51111) passed |
| 35380 | Bottom-right guards killed |
| 35850 | **[Star Rod]**, chest (276800, 89600) |
| 36111 | Mystery Juice used on Arche (max MP 55 → 57) |
| 36389 | Star Rod equipped on Sana |
| 37208 | Party healed. **End.** |

## Party at the end
- **Levels:** Arche, Sana and Stella are all **Lv18**.
- **Arche:** 303/303 HP, 15/57 MP. Org Slayer, Iron Armor, Leather Cape, Kitty Ears. Defense +1 from the Tough Drop.
- **Sana:** 236/236 HP, 81/291 MP. **Star Rod** (Atk 67→83, Spirit 97→105), Green Smock, Scale Armor, Chest Sash, Anti-Poison Ring.
- **Stella:** 265/265 HP, 119/259 MP. Lady's Dagger, Magician's Outfit, Woven Gloves, **Celebrity Ring** (Spirit 92→102, Resist 77→92).
- **Money:** 27817 CP.
- **Key items:** White Iron Ore (Help Out). No Pitch Black Key left.
- **Healing:** one Better Herb went on a quick-heal. Left: Better Herb ×3+1+1, Best Herb ×2, Revival Drink ×2, Revival Drink EX, Magic Candy ×3, Mind Chocolate, Recall Lamp ×2.

**Skipped:**
- Rune Staff and Magic Bracelet are now unequipped (replaced).
- I kept the Magic Candies; Arche's MP is low (15) for Unison next leg.

## Not reached
The 450260 exit is z2 (275200, 32000) at the top right → 450270, presumably "top right stairs down → 4-way junction".
- To get there: back left to the middle shaft (x ≈ 96000–105600), up it, then right along the top corridor.
- On the way: trap gates 70230 at 154200 and 218200 hold 2 Ice Dragons (677 / 699 HP). Near the exit: 51241 (253) and 2 Centurions (562 / 548). Also on the top corridor's left part: 2 × 51252 and 51083.

## Walkthrough corrections and game facts (EN-old)
- **450220 z3 → 450240** is the direct way in: the arched door between the background statues. Detouring via 450230 isn't needed.
- **450240:**
  - The real gargoyles are 70410 at 369600 (handle 450240511, already on plate 513) and 414400 (handle 450240512).
  - Push the right one left (sword sheathed) about 25600 onto plate 70800 at 384000. Fancy door zone 2 (377600, 89600) → 450245.
  - The skeleton is at about (47000, 71200) in the far-left side chamber, reached by a Unison boost from the bottom-left room. Its text: "Something is shining between the bones… examine?" → Yes.
  - Celebrity Ring ledge: top y 20000; boost from the upper floor at x ≈ 422000.
- **450245 (water room):**
  - Boat 70600 (slot 6) carries the party right at 200/frame when stood on, and stops at x 124800. Jump right onto the ledge (ly 23200).
  - With Mermaid Robe, followers walk underwater and stay behind. Heal Water II did not reach Arche from there.
- **450250:**
  - Mystery Juice is at the top right.
  - The top-left gate 70230 (handle 450250511) is a **monster trap**: it closes (state 1 → 0) when you approach and opens when the Ice Dragon + 2 Blazemages die.
  - Then the left shaft → bottom (stone blocks 70210, falling spikes 70430 that cost about 145 HP) → lock 70217 → gate → 450260.
- **450260:** Star Rod chest is at the bottom right (276800, 89600). It is guarded by 51242, 51241, 51122 and the bottom trap section between gates 163800 and 227800.
- **Mystery Juice** gives +2 max MP (the text says "permanently increases Max MP"). **Tough Drop** gives +1 Defense.
- **Healing:**
  - Heal Water II: press `n8` once, then hold X about 75 frames and release. A second `n8` press while the spell is already selected does nothing.
  - Quick-heal (F2) in battle takes over 200 frames to charge.

## Named state (run-local)
- `m0`–`m2`: party HP. `phmin`: their minimum.
- `e0`–`e23`: HP of hostile character slot N; 0 when dormant.
- `nfoe`: number of live hostiles.
- `boatx`: x of 450245's boat (slot 6 in world+0x11e0).

## Tool gaps and pitfalls
- **`rd tap ent --until "script == 0"` swaps the leader** when the V before it opened nothing. It happened twice; I undid both. A guard would help: stop if `script == 0` at the start.
- **Unison momentum carries movement.** `R*8 .*10` repeated counts as a dash double-tap, so stepping in front of a chest overshoots. A turn needs about 10 frames of held direction.
- **Fight searches with 3+ strong foes need 250–400 s.** The shell moves them to the background after 180 s.
- **Trap gates aren't flagged.** 70230 changing state 1 → 0 when you approach is a trap; `rd zones` could mark it.

No repo edits and no commits.
