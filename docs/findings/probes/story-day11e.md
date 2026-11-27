# probe story-day11e

run `story-d11e`; accepted 2026-11-26 18:08

check PASS (tools/drive/tasks/story-day11e.md)
verify MATCH (+ real device)
filed tests/scenarios/story-day11e/trace.jsonl (715348 frames)

## driver's report

# story-d11e: Archmage's Tower 5.3 → 5.6 (3 repair items got; stopped at the 40k cap before the bed)

Run `story-d11e` (--from story-d11d), RD_HOST=bunker throughout. Start f675176 (440220 bedroom) → end **f715348** in
**440130** (lower floor, entry 3 from 440140), control, foe 0, leader Arche. 40.17k frames.
Gates: `rd check` **PASS** (step 1 f675177, step 2 f675443, forbid clear). `rd verify` **MATCH**: frame 715348
rng f07c9f16 semv 7402f3da12f6a4df fb 6be9ba248fe47a8f (live = replay).
**No end save**: hit the ~40k cap in 440130 on the way back. The bed / slot save (step 5) is still owed. Last save is
still slot 7 `8b171418cd3b`. The next leg should start `--from story-d11e` (the run), not from a save.

## Route (frames)
| frame | what |
|---|---|
| 675442 | 440220 → 440210 |
| 676327 | 440210 zone 1 ↑ (25600, 51200) → 440200 (on arrival, "That old man was really angry" chat, f677178–677300) |
| 677917 | 440200 zone 3 ↑ (172800, 60800) → **440205** library |
| 678120 | V on lock 70217 (28800, 35200), facing right: "You used Archmage's Tower Key" → gate 70230 (34200) opens |
| 678413–678786 | event zone 101 (x 67200): library scene |
| 679027–679565 | **5th shelf** 70353 x 102000 (handle 440205516; shelves x 71200/79200/86400/93600/**102000**/116000/123200/130400): 'Magic Pot Repair for Beginners' → needs glue, Poton Sap, Rozack polish |
| 680152 | 440205 → 440200; 680874 lower-left door ↑ (25600, 89600) → 440190 (top floor) |
| 681291–681355 | 440190 lock 70217 (36800, 28800) V facing left → gate 70230 (31000) opens; 681539 x 0 → **440195** |
| 681579–682194 | 440195 "tiny ledges" = outside pillars (x ~131000 ly 32800; ~111000 ly 42400; ~90000 ly 29600) → left block ly 23200 |
| 682194–683073 | 440195 fight: Org Swordmaster + Org Bowman on the left block |
| 683260 | 440195 zone 2 ↑ (19200, 25600) → **440198** (Glue puzzle room) |
| 683300–688361 | Glue puzzle (below); 688361 lever 70216 pulled with **V** → gate 70230 (130200) opens |
| 688870–688931 | chest 70220 (147200, 41600) **[Glue]** |
| 689003–690084 | follow-mode mess (F3 presses dropped); fixed via F1 → Party Commands → Follow Mode → All Members 'Follow Me!' |
| 691498 | 440198 → 440195; 692950 back across the pillars → 440190 (x 0) |
| 693109–700069 | 440190: fights on the top floor (Org Bowman, Baby Aquamage, harpies), right shaft down, middle corridor left, left shaft down, bottom floor right (Pink Slime, harpies, Kobolds) |
| 700315 | 440190 zone 4 (214400, 108800 walk) → **440170** |
| 702320 | 440170: drop the pit, fights, zone 2 ↑ (89600, 73600) → **440180** |
| 702360–702779 | Sana casts Heal Water II (n8) ×2 |
| 703125 | 440180 lock 70217 (62400, 78400) V → "platforms start" (70202 lifts) |
| 703268–704318 | cross: tower edge → lift m0 → m1 → stone block → lift m2 → m3 → m8 → m4 → top-left ledge (fights: Org Bowman, Young Harpy, Kobold) |
| 705854 | Better Herb → Arche (menu, instant with the menu up) |
| 706341–706370 | chest 70220 (35200, 25600) **[Poton Sap]** |
| 706606–706850 | back down the lifts to the tower floor → door ↑ (16000, 86400) → 440170 |
| 707378 | 440170 zone 1 ↑ (16000, 73600) → 440150 (entry 3, x 278400) |
| 709050–709147 | (detour: went into 440160 by mistake, came back) |
| 710434 | 440150 Pink Slime killed; 710793 zone 1 ↑ (198400, 48000) → **440130** upper floor |
| 710988 | 440130 lock 70217 (184000, 22400) V facing right → gate 70230 (189400) opens |
| 711114 | 440130 zone 4 (201600, 12800 walk) → **440140** (upper corridor) |
| 711154–712965 | "leap of faith": run off the upper corridor's right end (x ~92800) → floating rocks x 131000–176000 (stand ly ~55000) → right ledge (x 200000+, ly 48800) |
| 713117–713150 | chest 70220 (216000, 51200) V **facing LEFT** (standing at 217036) **[Rozack Polish]** |
| 713589 | from the right ledge, down-left jump onto the lower corridor (x ~82000, ly 96800) |
| 713809–715202 | 440140 lower corridor fight (Kobolds 51031, 51241, Baby Aquamage) |
| 715289 | 440140 zone 1 (0, 86400 walk) → 440130 lower floor (201600, 96800); stopped at f715348 |

## Glue puzzle (440198): how it works [RD story-d11e]
- Four platforms 70420 on poles, in **two balance pairs**: P1 (59200) ↔ P2 (80000), neutral y 35200, range ±9600
  (down at 44800 = flush with the floor). P3 (92800) ↔ P4 (112000), neutral y 28800, range ±16000 (down 44800 / up 12800).
  The heavier side goes down; an equal weight returns to neutral (with nobody on it, the pair goes back to neutral).
  Block 70210 at (102400, 38400) on the floor. Lever 70216 at (113600, 8000). Gate 70230 at 130200. Chest at 147200.
- Solution used: Sana leader → F3 ×2 ('Stay There!') → Sana onto P1 (it sinks to the floor) → Arche (switched in;
  Sana stays) pushes the block LEFT from 102400 onto P1 (blocks it at o6x 59200, pushing Sana slightly) → Sana steps
  off, the **block keeps P1 down and P2 up** (standing ly 20000). Arche full-jumps from the floor at x ~110000 onto P4
  (neutral, ly 23200; Arche reaches it) → P4 sinks to the floor, P3 rises to ly 7200. Sana: floor → block top → P2
  (ly 20000) → P3 (ly 7200; a 1-vs-1 there did NOT move the pair). Stella: walked across P4 (pair went neutral),
  then onto P3 → 2 vs 1 → P3 down, **P4 up (Arche at ly 7200)** → Arche **V** on the lever (X with the sword did nothing).
- Followers stuck in a sunk platform's dip stay there until you switch to them and walk them out.

## Party at end
- **Arche** Lv15 261/280: Org Slayer, Scale Armor, Leather Cape, Kitty Ears (unchanged).
- **Sana** Lv15 145/209: Rune Staff, Scale Armor, Chest Sash, Green Smock, Anti-Poison Ring.
- **Stella** Lv15 158/237: Lady's Dagger, Scale Armor, Woven Gloves, Magic Bracelet.
- Gold 18003 CP (+2016 from kills). Better Herb: 2 used (now ~5), Best Herb ×4 untouched.
- Key items gained: **Glue, Poton Sap, Rozack Polish** (+ Archmage's Tower Key, used on 4 locks, still usable).
- No levels gained, no shops, no gear found on this leg. Not taken: the 440150 pillar chest (27200, 19200), off route.

## Walkthrough corrections (EN-old)
- 5.3: "dead-end wooden door" = 440200 zone 3 → **440205**: lock 70217 by the gate (V); library = event zone x 67200;
  the 5th shelf from the left = x 102000.
- 5.4 Glue: "back down, left, unlock gate" = 440200 bottom-left door → 440190 TOP floor (arrive at 108800) → left to
  the lock 70217 (36800, 28800) / gate (31000); "tiny ledges" = **440195**: outside, 3 stone pillars, then the left block
  (Org Swordmaster + Org Bowman); "stairs" = ↑ (19200, 25600) → **440198** = the puzzle room itself (see above). In the
  walkthrough wording, "Arche pushes block onto the leftmost platform while Sana/Stella holds it" = P1. "Highest" = P3.
- 5.5 Poton Sap: "back to area start" = 440190; "past stairs, right, drop pit" = top floor right → right shaft down →
  middle corridor LEFT → left shaft down → bottom floor RIGHT → walk-out zone 4 (214400, 108800) → **440170**; "drop
  pit" in 440170 = the hole to its lower floor; "right stairs down" = ↑ (89600, 73600) → **440180**. Key on lock 70217
  (62400, 78400) starts the 70202 platforms: lift m0 (x 91200, y 55000–79000), m1 (y 54400, x 109400–133400), stone
  block (stand ly 52000), lift m2 (x 192000, y 29600–53600), m3 (y 28800, x 154400–178400), m8 (y 22400, x
  115800–139800), m4 (y 28800, x 77600–101600) → top-left ledge chest (35200, 25600). The wooden scaffold beams are
  scenery: you fall through them (pit).
- 5.6 Rozack Polish: "back, left stairs down" = 440170 zone 1 → 440150 (x 278400); "left, stairs down" = 440150 zone 1
  ↑ (198400, 48000) → **440130** upper floor (NOT 440160; I went there by mistake); "key on lock to right" = 440130 lock
  (184000, 22400) / gate (189400) → walk-out zone 4 (201600, 12800) → **440140**; leap of faith = off the upper
  corridor's right end onto floating rocks → right ledge chest (216000, 51200) (harpies + Org Bowman/Kobolds). The way
  back = a down-left jump into the lower corridor → x 0 zone 1 → 440130 lower floor.
- Way back to the bed from 440130 lower floor (not done): 440130 → … → 440150 → 440170 → x 0 → 440190 (climb) → 440200
  → 440210 → 440220.

## Named state (run-local)
`h0..h2`, `hmin`; `px0..2 / py0..2` = party member positions (slot body +4/+8); `o1x..o6y` / `m0x..m8y` = object
positions (s32(s32(s32(world+0x11e0+4k)+0x40)+4/8), room-local slot k); `eh0` = HP sum of targetable enemies in slots 0–7,
`eh8` slots 8–15, `eh16` slots 16–23 (an 8-slot sum is the most that fits the 598-token limit; a 16-slot sum
does not).

## Tool gaps / lessons
- **`rd rewind` rebuilt the whole session (33 min replay)**. Use `ck load` / `undo` instead.
- `lstate` (0 stand, 3 rising, 4 falling) is the grounded test. Searches with `lpose == 0` or `--hold` alone accepted
  mid-air arrivals (an arrival at the top of a jump next to a pillar).
- F3 presses get dropped (the HUD icon went anchor → figure → flame inconsistently); the F1 → Party Commands → Follow
  Mode menu is reliable. The icons: anchor = Stay There, flame = Fight Freely.
- Herb from the menu: with no fight it applied at once; in a fight it became a timed cast ("Better Herb-Arche") and the
  closing `ent` swapped the leader. A `tap ent --until script==0` after a chest with no box swaps the leader too.
- A 1-frame turn: a short opposite press (<8 frames) only starts the turn; `L*10` turns without moving much.
- Fight searches run slowly (37–270 ticks/s) with many actors; the party AI (Sana Heal Water) matters: fights went
  much better once the followers had caught up (they warp in after a while when they are stranded).

