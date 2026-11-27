# probe story-day11g

run `story-d11g`; accepted 2026-11-27 00:18

check PASS (tools/drive/tasks/story-day11g.md)
verify MATCH (+ real device)
filed tests/scenarios/story-day11g/trace.jsonl (765295 frames)

## driver's report

# story-d11g report

The pot puzzle is solved: Arche learned Unison and Wind Thrust, picked up MoH 17 on the roof, and the game is saved. `rd check` PASS and `rd verify` MATCH on bunker.

**Run:** `story-d11g` (from `story-d11f`, `RD_HOST=bunker`). It starts at f741344 in 440240 and ends at **f765295** in the 440220 bedroom, in free control with Arche leading. That is 23,951 frames.

**Gates:**
- `rd check … story-day11g.md`: PASS. Step 1 "in control in the puzzle" at f741345; step 2 "left the puzzle room" at f752848.
- `rd verify`: MATCH. Both the live session and the headless replay end at frame 765295, rng ed32996d, semv 0abb20659f8b3186 (fb dca5b7ff4f041bf1).

**End save: `e65ce3a856fa`**, slot 9, on the bed after MoH 17. Slot 9 held "Barness Village Stella LV3 4:22", the shortest play time outside slots 1–8 and 24; nothing else was touched. Restart with `rd new NEXT --from save:e65ce3a856fa`. An earlier save to the same slot, before MoH 17, had digest `7fc678071ae2`.

## Vine link graph: there are no links
I read all 14 code-70400 actors from `world+0x11e0` before burning anything. They don't appear in the `world+0x2790` handle list at all. Every one has handle +0x1d8 = 0, link +0x298 = 0, +0x288 = 0 and +0x29c = 0, so `switch_link` never chains between them. Each piece burns only when a shot hits it directly; material +0x290 = 4 means "switch on when hit".

| actor slot | x, y | role | state at start |
|---|---|---|---|
| 8 | 70000, 14000 | under II | 0 |
| 7 / 16 | 62000, 9200 / 4400 | left of II | 0 |
| 9 / 11 / 27 | 74800, 9200 / 4400 / 12400 | right of II | 0 |
| 13 | 44800, 3600 | far left, irrelevant | 0 |
| 4, 5, 6, 12, 14, 15, 26 | various | already burned | 1 |

So the "spread" story-d11f saw was its shots' hitboxes and paths crossing several pieces. The guide's order ("burn around, not under") is simply a matter of hitting the right pieces.

## Route
| frame | what |
|---|---|
| 741473 | Stella, from on top of I, `R*4 C*6 RC*8 C*2 UX .*6 RX .*4` burns both left pieces; the under piece stays |
| 741586–741591 | Stella jumps off into the well firing `UX` while falling; this burns all three right pieces, the under piece survives, and she lands on the right ledge |
| 741849 | Arche, from the upper-right ledge (`L*44 LC*24 L*44`), lands on the under piece's exposed lip at lx 73316, ly 8400, and starts pushing II left |
| 741881–741961 | II leaves the vine once x reaches 63600 and drops straight down without snapping, to (63600, 54400) |
| 742304–742382 | Arche drops into III's cell (floor = top of VI) and pushes II left to **(60800, 54400)** ✓ |
| 745333–745533 | Arche climbs to the upper-mid ledge left of I and pushes I off the right edge: it snaps to **(54400, 54400)** ✓ |
| 746035 | F1 → Party Commands → Follow Mode → All Members 'Stay There!' |
| 745787 | Stella stands on the right balance platform; it sinks to 73600 and the left one rises to 48000 |
| 746633 | Arche pushes III from the far-right floor onto the right platform (112000) |
| 749806–749953 | Sana, then Stella, stand on the left platform; the right one rises and III ends up at (112000, 41600), level with ledge B |
| 750360–750602 | Arche stands on the jump-through at (118400, 48000) and pushes III onto ledge B, all the way to x 99300 |
| 751065–751349 | Stella, then Sana, move to the right platform; the left platform rises to 48000 |
| 751371–751771 | Arche pushes III left across the left platform and ledge A, and off A's left edge: **(67200, 54400)** ✓ |
| 751872 | All 9 blocks confirmed on their target cells by reading their positions |
| 752450 | Walking to the magician (Towarin) at the far left of 440240 (about x 12000) starts the scene |
| 752688 | "Did you fix my pot?": the tap that advanced the dialogue also answered this prompt (Yes, the default) |
| 752876 | Moves to 440210: "a passing grade", then the long Towarin talk |
| 759489 / 759511 | "Arche learned Unison!" / "Arche learned Wind Thrust!"; control returns at f759512 |
| 759723–759888 | Follow Mode set back: All Members 'Follow Me!', Arche 'Match party' |
| 760313–761565 | 440220 bed: nap, save to slot 9 (`7fc678071ae2`) |
| 762067 | ↑ at (19200, 35200) → 440230, the roof |
| 762148–762393 | Unison cast: Z to draw the sword, hold X about 105 frames, release; 10 MP |
| 762624 | Boost-jumps up to **MoH 17**, 70100 at (102400, 23235); "Congratulations" ends at f763544 with a full heal |
| 764084–765295 | Back to 440220, nap, overwrite slot 9 (`e65ce3a856fa`) |

**Skipped:** the [Memory Photo 7] chest on the "plank scaffolding". Room 440230 has no chest object; its only objects are the MoH, two torches and the door back. Its location is unknown, so I stopped at about 24k frames.

## Party at the end
- **Levels:** Arche Lv16, Sana Lv15, Stella Lv15, all at full HP/MP.
- **Money:** 18856 CP.
- **Gear:** unchanged from story-d11f. Arche: Org Slayer, Scale Armor, Leather Cape, Kitty Ears. Sana: Rune Staff, Scale Armor, Chest Sash, Green Smock, Anti-Poison Ring. Stella: Lady's Dagger, Scale Armor, Woven Gloves, Magic Bracelet. No shops or chests on this route.
- **New:** Unison and Wind Thrust are learned and set to "Use" (Abilities, page 3). MoH 17 raises the level cap.

## Walkthrough corrections (EN-old, step 5.7 / 5.8)
- **Vines:** there are no links. Burn the left pair with a jumping `UX`/`RX` from on top of I, and the right trio with `UX` fired while falling into the well. Nothing burns the piece under II unless it is hit.
- **II:** there is no need to burn under it.
  - Arche stands on the under vine's exposed right lip and pushes II left. It drops straight down once its right edge passes the vine (x 63600).
  - Then she pushes it the last 2800 to 60800 from inside III's cell, which is the "nudge".
- **I:** pushed right off the upper-mid ledge, it snaps to its spot directly.
- **III:**
  - The platforms move ±12800. On the left platform, Sana alone weighs as much as III; Stella alone weighs less.
  - Push III to the left end of ledge B (x 99300) *before* moving the girls to the right platform. If it is left at 105600, there is no ledge on its right for Arche to stand on.
  - Arche pushes from the jump-through at (118400, 48000).
- **Magician:** he is at the far left of 440240 (x about 12000, lower floor) once all blocks are placed. The "Did you fix my pot?" prompt is Yes.
- **Unison:**
  - It does not go in a spellbook (its Abilities entry offers only Use / Don't Use), and nothing shows on the HUD.
  - Cast it with the sword drawn: hold X for about 105 frames, then release; it costs 10 MP.
  - In the air, C presses chain the boost jumps.
- **MoH 17:** 440220 zone 2 → 440230 roof, MoH 70100 at (102400, 23235).

## Game quirk seen
A 'Stay There!' follower left on a balance platform stayed at the platform's old height after the platform moved away. Stella hovered at (97648, 42400) and didn't count as weight. She fell only after I made her leader and moved her.

## Tool notes
- `rd search` with `--hold` was the key tool. Without it, a goal reached mid-shot was later undone (119 such arrivals rejected), and each try would have looked like spread.
- `rd text` often misses short system lines while they are still typing ("Overwrite this save?", "Game saved."). I confirmed both saves with `rd look` on the slot list and a new `rd endsave` digest instead.
- My first guess at the party member variables was wrong: slots in `world+0x11e0` are objects, while party characters are `world+0x1160` slots 0–2.
- `rd verify` rebuilt the session (about 37 min), so this run's checkpoints are gone.
- **Run-local names defined:** `vl1 vl2 vu vr1 vr2 vr3 vfl` (vine states), `b1x/b1y b2x/b2y b3x/b3y` (I/II/III), `plx ply prx pry` (platforms), `ax ay sx sy tx ty` (party), `moh`.
