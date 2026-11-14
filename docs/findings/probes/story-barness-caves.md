# probe story-barness-caves

run `story-bc`; accepted 2026-11-14 09:32

check PASS (tools/drive/tasks/story-barness-caves.md)
verify MATCH (+ real device)
filed tests/scenarios/story-barness-caves/trace.jsonl (23751 frames)

## driver's report

## story-bc: through Barness Hills to the "end cave" block puzzle (280150); the dungeon was not reached

**Result:** `rd check` PASSES for tools/drive/tasks/story-barness-caves.md (step 1 at frame 779, step 2 at frame 3001, the forbid never fired). `rd verify` says MATCH: frame 23751, rng efcb56b3. The live session had drifted by one volatile dword, and the clean replay matches (evidence in runs/story-bc/verify-drift-f23751). The movie is **23751 frames** long. I stopped well short of 100k frames because the API budget was running out, not because of a hard block.

**The last driver's blocker, solved:** room exits are event zones, and a nearby enemy holds them shut. From the decompile (src/gimmick.c zone_pass, 0x59a1f0):
- A zone fires only when no hostile is within 56000 × 20000 of the leader (48000 once the zone's event is done), or near any active party member.
- Each time a hostile is near, the zone's timer is reset to 30 ticks, so the zone fires only after 30 hostile-free ticks.
- Kind 1 zones fire when you walk in. Kinds 2–3 fire on an ↑ press while you stand idle on the ground.
- The HUD "ESCAPE" icon shows when you stand in a zone that is being held. The red markers at the screen edges point at off-screen enemies, not at doors (rd_4969b0 scans characters).

**Route (frames):**
- f779: Continue → slot 7 → 270200, the inn front. At f945 I stayed at the inn by mistake (6 CP) and cancelled its save prompt (nothing written).
- f2239: archway in 270120 → 280110, Chiffon scene. Control at f2760.
- **280110 has only one exit besides the way in:** a walk-in zone at its west edge (x 0–3200, y 32000–48000) leading to **280120**. It needs the Mothbees and Cocorats near it killed. f5357: in 280120.
- 280120 (a hanging basket lift) has ↑ doors: x 147200 → **280130**, x 19200 → **280125** (not visited, probably the "2nd cave").
- f7523: in 280130 (the guide's "1st cave"). Its exit is at the bottom (x 105600, y 153600) → **280140**. I did not get the Better Herbs.
- f8790: in 280140, outdoors. Its ↑ doors: x 185600 → **280145**, x 374400 → **280150**.
- f10315: game over (coverage, seen once), at HP 5 with no items. I picked Return To Safety: 270120 (the retry room is `s32(rmo+0x40d0)`), HP full, 892→815 CP.
- I found no item shop. The inn's right door leads to 270205, a bedroom with a travelling nun.
- Second pass, with searches that avoided low HP: 280120 at f14149 (HP 143), 280130 at f15333, 280140 at f16966, 280145 at f18392.
- 280145 is the herb cave. Its chests are at the bottom-left tunnel (lx ≈ 27000, ly 87200), opened with V: **Herb** (f19049) and **Strongirl Juice** (f19217). F2 quick-heal used the Herb: 42→122 HP.
- f21967: in **280150**. Its exits: the entry (→280140), x 176000 → **280115** (the gated cave back toward Barness), x 16000 → **280155**.
- In 280150, stone blocks with a crest stand on both sides of the entry, with a pool to the left. The door to 280155 sits on a ledge above the left pool, and I could not climb out to it.
- **Pushing only works with the sword sheathed** (Z). While pushing, `lpose` = 114 and she moves 100 per frame. With the sword drawn, she just stops at the block.
- f22885–23751: I pushed the left block right until it hit the block on the right of the entry (lx 145092). Two blocks in contact don't move. **Next step:** leave and re-enter 280150 (presumably resets the blocks, unverified), then with the sword sheathed push the RIGHT block first, toward the right-hand water past x 176000, then the others (guide step 7).

**End-save digests:** none new. I passed no goddess statue and cancelled the inn's save prompt. To continue from here, replay this run (`rd new X --from story-bc`), or start from `end:story-barness-hills` and follow the room chain above.

**Named state** (defined in this run):
- `zunder` = `s32(s32(s32(world+0x1044)+0xf8)+0x274)`: the zone id under the leader (0 = none). It read 1 on arrival at each door and 2 or 3 at the other doors. Matched the actor scan.
- `zrest` = `s32(s32(world+0x1044)+0x104)`: the hold timer. It is 30 while a foe is near; at 0 the zone can fire. Seen going 30→0 as I walked away.
- `foehp` = the sum of HP of alive chars with faction 2 in world chars slots 0–5. Each char is `c = s32(world+0x1160+4i)`; alive = `c+0x1d0`, faction = `c+0x1dc` (1 = party, 2 = enemy), HP = `s32(s32(c+0x750)+0x54)`, x = `s32(s32(c+0x40)+4)`. Validated when a Yellow Slime's HP went 69→52 on a hit.
- `threat` (slots 0–3 within 62000 in x) and `foenear` = world+0x2774. Both are rough.
- How to list a room's doors:
  - The zone table is at `s32(world+0x1038)+0x1c`: 20 records of 12 bytes, {zone id, target room, kind?}.
  - Zone actors are entries `a = s32(world+0x11e0+4k)` with zone `a+0x274` ≠ 0, kind at `a+0x278`, and box `s32(a+0x40)+4/8/12/16`.
  - This is how I found the 280110 west exit.
- Also seen: difficulty `s32(world+0x2780)` = 10 (Easy), defeats `s32(rmo+0x40b0)` = 7.
- `hp`, `mp`, `gold`, `party` from the last probe all held. The 892→815 game-over loss matches −10% truncated.
- **Day counter: not found.** There was no day change in this leg. The candidate stays story flag 100100101 (= 1110 on Day 6). The 280170 script checks it, together with the entry door 280170501, so 280170 is probably the pedestal / dungeon-door room.

**Walkthrough corrections (EN-old):**
- The 280110 caves are not doors in the field. The field's only onward exit is the walk-in west edge to 280120, and only once nearby enemies are dead.
- Room map: 280120 (lift hub) → 280125 (unvisited; 2nd cave?) / 280130 ("1st cave", exit at the bottom) → 280140 (outdoors) → 280145 (herb cave) / 280150 ("end cave").
- The herb chest gave one "Herb", not ×3.
- The block puzzle needs the sword sheathed.

**Tool gaps:**
- `actors()` is not mapped for this game, and a 598-token cap applies to stop expressions. That made any "foe near" predicate awkward.
- `rd search` with default macros keeps re-pressing ↑ and leaves the room by accident. I needed `room == X and ...` goals with `room != X` in the avoid.
- One search session failed with "err anchor failed" and wedged the live session until `rd stop`.
- Short presses (R*5 to R*12) sometimes don't move Arche after an attack or a turn. `toward()` overshoots.
- There was no quick way to read the inventory count.

---
model claude-opus-5-5, effort high, 254 turns, $6.60, 134 min

