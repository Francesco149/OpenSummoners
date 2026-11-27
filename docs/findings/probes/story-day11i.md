# probe story-day11i

run `story-d11i`; accepted 2026-11-27 03:42

check PASS (tools/drive/tasks/story-day11i.md)
verify MATCH (+ real device)
filed tests/scenarios/story-day11i/trace.jsonl (43827 frames)

## driver's report

# story-d11i report

The party is inside the **Labyrinth of Night** (room 450110), but this leg did not reach step 7.1: no [Pitch Black Key] and no MoH 18. The ~40k frame budget ran out on the way.

The run does **not** end on a save. The last save is at the White Rose Inn (f25241), about 18.6k frames before the end. Start the next leg from the run itself with `rd new NEXT --from story-d11i`, which replays only these 43.8k frames. Starting from the save instead would repeat the walk from 265160 to school.

**Run:** `story-d11i`, started from `save:dfd67b2a393b` with `RD_HOST=bunker` throughout. It ends at **f43827** in 450110, the labyrinth entrance hall, in free control with Arche leading. That is 42.9k frames past the start (f885).

**Gates:**
- `rd check story-d11i tools/drive/tasks/story-day11i.md`: **PASS**. Step 1 "in control" at f886, step 2 "left the Guardpost" at f1912, no forbid hit.
- `rd verify story-d11i`: **MATCH**. Live session and headless replay both end at frame 43827, rng 9f348f76, semv 1ff8f18d43c4d599, fb 9cf6a01b81830f23.
- I looked at the end frame (party in the dark brick hall by the double door): `/mnt/ephemeral/opensummoners/drive/runs/story-d11i/view/look_f043827.png`. The session is stopped.

**End save: `21d1362d3bba`**, slot 11 "White Rose Inn, Arche/Sana/Stella LV16, 10:14", made after resting. Slot 11 held "Town of Tonkiness LV6 5:09", the shortest play time outside slots 1–10 and 24. Use it only if the run replay is not wanted: `rd new NEXT --from save:21d1362d3bba` starts at 265165.

## Route
| frame | what |
|---|---|
| 885 | Continue → slot 10 (page 2, cursor moved up from 15). The title cursor started on Bonus Menu (`tcur` 2); one wrong pick was undone. In control in 430120. |
| 1029 / 1911 | 430120 → 430110 → zone 1 (252800, 25600) → 420180 |
| 3687 | 420180 → 420170, fight search, 1715 f |
| 6891 | 420170 → 420160 via the top-right exit (252800, 12800); a 51232 held it |
| 8927 | 420160 → 420150 (cave) |
| 10634 | 420150 ↑ at x ≈ 315500 → 420120 |
| 12236 | 420120 → 420110 |
| 14638 | 420110 ↑ at x ≈ 277500 → **Chartreux 300110** |
| 15446–16850 | Weapons 300220: **bought Iron Armor 9800 for Arche** |
| 18184 | 300120 gate ↑ at x ≈ 283200 → 265110 |
| 19677 → 21166 | 265120 → 265130 → 265140. The 265130 exit (zone 3, x 92800) is held by slimes at the right wall. |
| 22746 / 23732 | 265150 → 265160 |
| 24057–25523 | White Rose Inn 265165: Rest 7 CP → save, slot 11 |
| 26171–26635 | 265160 rose bush: **Stella leading (`ent` ×2) burned through in a 464-f search**; `ent` once switches back to Arche |
| 26909 → 31553 | 265170 → 260190 → 260180 → 260150 → 260140 → 260130 → 260120 → 260110, every hop heading right |
| 34578 | 260110 → **Tonkiness 210110** (2986 f, the slow hop) |
| 35196 | 210110 zone 7 (↑ at x ≈ 267200) → 225110 |
| 37475 | 225110 pit gap at x 102400–118400, crossed with a dash-jump search |
| 38207 / 38896 | 225120 → 225130 |
| 40501 | 225130 → school yard **230110** |
| 40517–41194 | Yard cutscene starts by itself (Arche: "why it's all dark around the school?"); Chiffon: "get inside" |
| 41534 | 230110 zone 2 (↑ at x ≈ 158400) → **450110, Labyrinth of Night** |
| 41534–43827 | Arrival cutscene ("this is a dungeon!", statues of Colm and Leon, "Let's go a bit further in."), then control |

## Party at the end
- **Levels:** Arche Lv16 (237/281 HP, 51 MP), Sana Lv16 (212/212), Stella Lv16 (240/248).
- **Money:** 10755 CP.
- **Arche:** Org Slayer, **Iron Armor** (Def 115 → 125), Leather Cape, Kitty Ears.
- **Sana and Stella:** gear unchanged (Sana: Rune Staff, Scale Armor, Chest Sash, Green Smock, Anti-Poison Ring; Stella: Lady's Dagger, Scale Armor, Woven Gloves, Magic Bracelet).
- **Items:** I did not re-check the bag; nothing was used or bought except below.

## Purchases and skipped upgrades
- **Bought:** Iron Armor 9800 (Arche only; "Cannot Equip" for Stella). Rest at the inn, 7 CP.
- **Skipped:**
  - Bronze Gauntlets 2800: on Arche, Def +6 but Resist −6, so not a clear upgrade; Stella can't equip them.
  - Magic Dagger 9700 ("for magicians"): affordable only by dropping below the 3000 CP floor after the armor.
  - Battle Sword 18000: unaffordable.
  - The 420125 flower-field Unison chest (1500 CP + Stronggirl Juice): the frame budget.
- **Shop screen:** the weapon shop shows no stat preview in the list. The comparison only appears after `ent` on an item, in the "On who?" box. `bk` backs out of that box without buying.

## Game facts and walkthrough corrections (EN-old)
- **School entrance = the labyrinth:** 230110 zone 2 (the 230130 door) leads straight to **450110** once Day 11 is over. The darkness scene plays in the yard first, starting on its own a few frames after arrival.
- **450110 exits:**
  - Zone 1 ↑ (144000, 28800) → 999999 (the school door; not tried).
  - Zone 2 ↑ (28800, 54400) → 450120.
  - Zone 3 ↑ (275200, 54400) → 450120.
  - Six 70371 objects (the statues) along the hall.
  - No save point in 450110.
- **Chartreux weapons 300220:** stock page 2 = Battle Sword 18000, Magic Dagger 9700, Scale Armor 4800, Iron Armor 9800, Bronze Gauntlets 2800. The keeper answers X with Arche facing left at lx ≈ 64600–67400.
- **White Rose Inn 265165:** the innkeeper is the woman at the right counter (lx ≈ 61000, facing right). Rest 7 CP, then "Would you like to save?" (default Yes).
- **225110:** there is a pit gap at x ≈ 102400–118400; a dash-jump is needed.
- **The 265160 rose bush** grows back again. Its 11 pieces of 70400 sit at x 202800–211200.

## Named state (run-local)
- `m0`, `m1`, `m2` = party HP, from `s32(s32(s32(world+0x2784)+0x4030+4*i)+0xa64)`.
- `phmin` = their minimum.

## Tool gaps
- **Savestate pool ran out mid-leg (~30 checkpoints).** The search and the chain failed until I dropped old ones. A warning before the pool is full would help.
- **Searches overshoot ↑-doors.** A score on `lx` alone carries Arche past the door (twice, by 10–30k). Walking back with `toward` and pressing ↑ after the slide stops was needed.
- **Waits on `lstate == 0` stall.** A `toward(...)` hold plus `lstate == 0` as the stop condition never stopped (300 f wasted); stop on distance only.
- **Searches are inefficient on easy rooms.** Paths are often 1000–3000 frames where a straight dash would take a few hundred. 225110 wasted ~2000 f before I spotted the pit gap.
- **Not reported anywhere:** an exit search does not say when the leader is stuck in a pit.
- **Scratchpad helpers I wrote** (in `/tmp/claude-1000/-opt-src-OpenSummoners/0553de8c-f334-4cbb-a0d1-f526e0ceeca7/scratchpad/`):
  - `f2.sh`: an exit and fight search that avoids low HP on any member, with an `EXTRA` score term.
  - `chain.sh`: multi-room hops that pick the 3 hostile slots nearest the exit and save a checkpoint after each room.

No repo edits, no commits.
