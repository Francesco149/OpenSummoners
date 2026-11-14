# probe story-barness-dungeon2

run `story-bd2`; accepted 2026-11-14 14:36

check PASS (tools/drive/tasks/story-barness-dungeon2.md)
verify MATCH (+ real device)
filed tests/scenarios/story-barness-dungeon2/trace.jsonl (65958 frames)

## driver's report

## story-bd2: I got through 290150 and pulled the 290152 lever, but stopped before riding the weight

**Status:** The run is `story-bd2` and ends at frame 65958 in room 290150, below the weight shaft at lx 214984, ly 80800. HP is 161/172 and gold 797. The budget ran out, so I did **not** run `rd check` or `rd verify`. The task's two steps should hold (control after the load; room ≠ 0 after frame 3000), and the forbid never fired. I passed no goddess statue, so I made no saves and there are **no end-save digests**.

**Next step:** a Kobold and a Killer Bat are near the shaft and will hold the exit zone, so kill them first. Then wait for the weight at its lowest point, ride it up, and step onto the upper-right platform. Zone 2 of 290150 is a walk-in at x 252800, y 44800–60800 and leads to 290155 (probably the guide's "fancy doors"). After that comes the gargoyle question (answer 12), then Stella. The weight's actor was not among the slots I scanned (`world+0x11e8`…`0x121c`); its object flag is 290150511.

**Checkpoints:** `r150` (43115), `gameover` (46186), `safety` (46314), `shopped` (47761), `r160`, `r110`, `blk1`, `stack`, `pushed`, `r130b`, `r150b` (59044), `r152` (60720), `edge`, `onmb`, `riding` (62757), `lever` (64092), `back150` (65279).

**Route (frames):**
- f43115: entered 290150 with HP 25, after a search through the 290130 fight.
- f45824: game over on purpose, since there were no herbs and HP was 12.
  - "Try Again" puts you back at the 290130 stairs door with the HP you entered 290150 with (25), so it does not help.
  - "Return To Safety" (f46314) sends you to 270120 with HP 172 and takes 10% of your money (996 → 897).
- f46829–47761: general store 270190. I bought 5 Herbs and 10 Better Herbs. Some of those Herbs were accidental `ent` taps.
- f48958: 270120 zone 5 (x 16000, kind 2) → 280115. This is the shortcut.
- f51492: 280115 → 280160. The door is zone 2 at x 124800, y 32000, under the rock arch.
- f53479: 280160 → 280170.
- f53812: 280170 zone 2 (x 38400) → 290110.
- f54271: 290110 → 290120. **The blocks had reset**, so I redid the puzzle: floor block to 108800 (f55267), ledge block dropped onto it (f55975), stack pushed to 153600 (f56708).
- f57922: 290130. f59044: 290150.
- f60720: **290152**, zone 3 of 290150. It is at the TOP-LEFT (x 16000, y 22400, ↑), not the "top right" the guide says.
- 290152 puzzle:
  1. Break the 3 crates with the sword.
  2. Push the stone block (start x 227200) LEFT. It climbs the stairs to the door landing.
  3. Push it off the landing edge (about x 157000) when the moving block is at its right end (f62164). The moving block runs x 114000 ↔ 153400 at 200 per frame and pauses about 20 frames at each end. The stone then rides it (f62184).
  4. Step onto the double block at mbx ≈ 153000, ride left, and long-jump to the upper-left ledge (f62851).
  5. Kill the Kobold. The chest 290152601 at x 19200 holds a **Better Herb** (f63917).
  6. The lever 290152501 is at x 12800 (f64015). It shows "In the distance, you can hear the sound of machinery activating." and clears object flag 290150511, the weight. I checked that in chapter_scripts.c:28667.
- f65229: back in 290150. The weight now moves.

**Named state** (defined in this run):
- `fh0`..`fh6`: enemy HP. For c = `s32(world+0x1160+4i)`, the HP is `s32(s32(c+0x750)+0x54)`, counted when `+0x1d0` ≠ 0 and faction `+0x1dc` == 2. `foehp` = their sum [status].
  - Validated: values fell to 0 as kills happened, and Arche's own slot read the HUD HP.
  - The `+0x750` party object comes from wl_party in world_layout.h.
  - Enemy x / y = `s32(s32(c+0x40)+4/+8)`.
  - The slot order changes from room to room.
- `sword` = `s32(s32(s32(lslot+0x9f4)+0x750)+0xd4)`: 1 = drawn. It comes from scene_cmd.c and I validated it with Z.
  - `lpose` 3 = drawing, 4 = sheathing. A hit cancels both, and Z pressed during hit-stun is dropped.
  - With the sword sheathed, X examines (`lpose` 2) and does no damage. That is why my first fight searches got nowhere.
- `b1x`, `b2x`, `b2y`: the 290120 blocks, actors `0x11e8` and `0x11ec`.
- `mbx`, `sbx`, `sby`: the 290152 moving block (actor `0x11e0`) and stone block (`0x11ec`).
- From vars.json, unchanged: `hp`, `mp`, `gold`, `party` (= 1, Arche alone).
- Day: there is no day counter; story progress is still `story` (flag 100100101).

**Game facts:**
- F2 quick-heal with herbs in stock starts a cast bar that finishes in about 200 frames.
- In a shop, after "Will there be anything else?", `ent` opens Buy/Sell. `bk` on Buy/Sell gives "Come again", and `ent` closes it. An `ent` in the Buy list buys at once.
- The 280115 → 280160 door holds while Yellow Slimes are within range.

**Walkthrough corrections (EN-old):**
- 8.5: the moving-block room (290152) is reached from 290150's top-left stairs, not "top right".
- 8.5: the stone block is pushed up the stairs and off the landing; the crates must be broken first.

**Tool gaps:**
- There is no actor, zone or enemy list expression, so I had to probe each slot by hand.
- The search output truncates long paths, so I could not apply a best path I had already found and had to re-run the search.
- Several fight searches ran slowly (200–600 ticks/s).
- `rd scan` replays from boot after checkpoint loads.

---
model claude-opus-5-5, effort high, 282 turns, $8.12, 107 min

