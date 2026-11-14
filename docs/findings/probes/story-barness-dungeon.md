# probe story-barness-dungeon

run `story-bd`; accepted 2026-11-14 12:07

check PASS (tools/drive/tasks/story-barness-dungeon.md)
verify MATCH (+ real device)
filed tests/scenarios/story-barness-dungeon/trace.jsonl (42256 frames)

## driver's report

## story-bd: from the block puzzle to the Barness Hills Dungeon (290110 → 290120 → 290130), stopped mid-fight

**Status:** The budget ran out before I could run `rd check` or `rd verify`, so neither has been run. The movie ends at about f42256. By my reading the task's steps hold: there was control after the load, and the room was 280150 → … → 290130 after frame 3000. The forbid never fired: I never went back to the title. I took no goddess-statue save, so there are **no end-save digests**.

**Where it stands:** Room 290130, HP 40/172, no healing items left (F2 does nothing). One Cocorat is still near the 290150 stairs door (slot `fh3`, 76 HP). The other one (`fh4`) is dead. Checkpoints in the run: `start`, `reset1`, `puzzle2x2`, `edge`, `lever`, `t115`, `r160`, `dungeon` (f36172), `r120`, `blk1`, `stack`, `up120`, `r130`, `door150` (f41728).

**Route (frames):**
- f23961: killed the Killer Bat holding the 280150 entry zone.
- f24118 → 24225: left and re-entered 280150. **This resets the blocks** (confirmed).
- I pushed the 4 blocks in order 4, 3, 2, 1 (rightmost first, after jumping over the others). Each falls into a 2-wide, 2-deep notch at the left edge of the right-hand pool, making a 2×2 slab flush with the floor: f25573, f26162, f27574, f29004. Pushing a block into a neighbour first jams both; I re-entered the room to fix that.
- Deep water acts as a pit: −8 HP and back to the ledge. Arche cannot swim.
- The far shore (x ≥ 257700) is about 31600 past the slab's edge. A plain dash-jump covers only about 22000. `rd search` found a crossing (f29188) that chains **air Jumpkicks (`R U X`) during a long jump**. The way back was found the same way (f29667).
- f29458–29506: the **heart lever is actor 280150501 at x 289600**, at the far right of 280150. Examining it shows "In the distance, you can hear the sound of machinery activating." and sets object flag 280115502 = 1.
- f29983: 280115 (zone 2 of 280150, x 176000). Its heart gate is now open. Zones: 1 → 270120 (the shortcut), 3 → 280150, 2 → 280160 at x 124800 (the "tunnel").
- f30184 / f30225: two F2 quick-heals (36 → 108 → 172). These were the last healing items.
- f31477: killed 3 Killer Bats. f31640: in 280160.
- f32905: 280170 (zone 2 of 280160, at x 35200 low). Kobold/slime there killed first.
- f33410–35343: the altar at x ~55600, opened with V/U. Chiffon scene "pwace the elemental stone on the altar", Arche places her stone, and a secret door appears.
- f35499: **290110, Barness Hills Dungeon** (control at f36172).
- f36309–36352: diamond switch 290110501 at x 12800 (left). It sets 280115501, the shortcut gate.
- 290110 zones: 2 → 290120 (↑ stairs at x 166400 on the lower floor ly 45600), 3 → 290200 (x 201600, right).
- f37156: killed a Cocorat holding the stairs. f37279: in 290120.
- 290120 zones: 1 → 290110, 2 → 290130 (walk-in at x 252800, y 12800, top right), 3 → 290250 (x 19200, probably the MoH 5 door).
- 290120 block puzzle, solved:
  1. Push the floor block (start x 80000) right to x **108800**, under the right end of the upper-left ledge (f38502).
  2. Climb onto that ledge (f38797) and push the ledge block (86400, y 16000) right. It drops onto the floor block (f39097).
  3. Push the stack to the right wall (block x 153600, f39764).
  4. Jump up to the upper route (f40222).
- f40699: killed a Kobold near a gargoyle statue. f41130: in **290130**.
- 290130 zones: 2 → 290140 (169600, 28800; the guide's optional chests) and 3 → **290150** (150400, 80000; ↑ stairs on the bottom floor ly 77600). Reached the bottom at f41728, but Cocorats hold the door.

**Next for a follow-up run:** kill the Cocorat `fh3` (or take a game over at 270120 for full HP; the shortcut gates 280115501 and 280115502 are now open). Then ↑ at x 150400 on the bottom floor → 290150 (guide 8.5: moving block / weight), then the gargoyle question (answer 12), then Stella.

**Named state** (defined in this run):
- `fc0..fc5` = `s32(world+0x1160+4i)`: char slots.
- `fh0..fh5` = HP of an alive slot whose faction (`+0x1dc`) is 2. `foehp` = their sum [status]. Validated: it fell to 0 as enemies died and matched the HUD target bars.
- `flags` = `s32(s32(world+0x2784)+0x40ac)`: the flag store. It holds two tables:
  - Story flags: records of {id, value, 0} from `flags+0`, count at `flags+0x1800`.
  - Per-object flags (chests / levers / doors): records from `flags+0x1804`, count at `flags+0x7804`.
  - Validated: `script_flag` in src/script_engine.c reads the story table. After the lever, the object table shows 280115502 = 1 and 280150501 = 1.
- `story` = story flag 100100101, read at `flags+0x4c` (guarded by its id at `flags+0x48`). It is **1110 on Day 6** and is the value the 280170 altar script checks. It did not change through the dungeon entry. **There is no separate day counter.** I believe 100100101 is the story-progress counter (others nearby: 100100102 = 30, 100100201 = 20).
- `b1x`, `b2x`, `b2y`: the 290120 block positions (actor `+0x40` → box x/y). Only valid in that room.
- `hp`, `mp`, `gold`, `party` from the earlier probes held: gold 853 → 993, HP matched the HUD, party = 1.
- Sword-drawn flag: **not found**. The diffs were too noisy. Pushing needs the sword sheathed; tell by `lpose` 114 vs 0 at the block.
- How to find doors and levers in a room:
  - Zone table records at `s32(world+0x1038)+0x1c`: {zone id, target room, kind}.
  - Actors at `s32(world+0x11e0+4k)`, with the box at `s32(a+0x40)` +4 / +8 / +12 / +16 (x, y, w, h).
  - Zone id at `a+0x274`, kind at `a+0x278`, handle at `a+0x1d8` (the lever is the one with handle 280150501).
- Arche's ly sits 5600 above her feet. Floor tops = block y + 6400.

**Walkthrough corrections (EN-old):**
- 7: the heart lever is **across** the right pool, not reached by walking on the stones. The stones only shorten the gap; crossing it needs a long jump with air Jumpkicks.
- The "heart wall" is a gate in 280115, the door at x 176000 of 280150. Its "tunnel" leads to 280160 → 280170 (the altar). The altar is examined, not walked into, and Chiffon explains the stone.
- Blocks reset on re-entry. A pit or deep water costs −8 HP.
- 8.1 is correct: the diamond switch sits at the far left of 290110 and the stairs down are on the lower floor at x 166400.
- 8.3: put the floor block exactly under the right end of the upper-left ledge. The stack is then pushed to the right wall. Getting onto the ledge needs jumps from the left side.

**Tool gaps:**
- There is no expression that lists actors, zones or enemies; I had to write per-slot names and eval by hand. No tuples or list comprehensions in `eval`.
- No named sword-drawn flag and no inventory count; I only found out the heals were gone when F2 failed.
- Momentum after `R*N` makes it hard to stop on a spot. Searches with `V`/`U` macros worked better.
- Fight searches are slow (200–300 s) and spend a lot of HP. A heal or HP budget per fight would help.

---
model claude-opus-5-5, effort high, 264 turns, $8.21, 88 min

