# probe story-stella-interlude

run `story-si`; accepted 2026-11-15 09:59

check PASS (tools/drive/tasks/story-stella-interlude.md)
verify MATCH (+ real device)
filed tests/scenarios/story-stella-interlude/trace.jsonl (99334 frames)

## driver's report

## story-si: Stella got out of Mayberk Manor and through the Chartreux east gate; stopped in 265120 at frame 99334

**Status:** `rd check` PASSES (step 1 at f779, step 2 at f3001, the forbid never fired) and `rd verify` gives MATCH (live and headless replay agree on frame, rng and semv). I stopped at f99334 because the card's limit is about 100k frames. That point is Stella solo in **265120**, the second road room east of Chartreux, with control, HP 59/81 and MP 45/51. Checkpoint `end` is there.

**End-save digest:** `944f0d1ef279`, the same as the previous run. I did not reach the White Rose Inn or any goddess statue, so no new save was made.

**Route reached (frames):**
- **f89312:** start on the upper floor of **310120** (y 20000).
- **f90468:** the upper door (zone 4, x 76800) → **310130**, the hall.
- **The way down (the blocker from last time):** walk LEFT in 310130. A staircase at the far left takes you down to the middle floor (ly 45600, f90710–90810). Walk RIGHT to the far right end (x ≈ 155000–177000), where a second staircase goes down going left. That reaches the ground floor (ly 71200) at f91575.
- **f92012:** the ground-floor hall door (zone 1, x 67200) → the ground floor of 310120.
- **f92329:** the 310120 door at x 25600 (↑) → garden **310110**, arriving at x 387200.
- **f93527:** walk left to x ≈ 118400 and press ↑ → **300130**, arriving at x 144000.
- **Scene and tutorial:** first Stella's "get my hands dirty" line, then a combat tutorial prompt (I chose Yes for coverage). At the end the tutorial asks "go over it one more time?". Choose No with `.*20 D .*14 ent`: a D pressed too soon after the menu opens is dropped and Yes loops the whole tutorial. Control at f94176.
- **f94596:** the 300130 right edge → **300110**.
  - An NPC girl on the bridge (x ≈ 257000) blocks walking; a run-jump `RC*14` clears her.
- **f95501:** x 382000, zone 6 (↑) → **300120**.
- **f96291:** the city gate (zone 6, x ≈ 281700, ↑ while idle) now OPENS → **265110**, the road east (outside the gate).
- **f96466:** Stella's first Fire Wall, cast and seen in a picture.
- **f97923:** Blue Slimes near the 265110 right edge hold the exit (the ESCAPE icon is crossed out). A fight search with Fire Wall and dagger cleared them.
- **f99294:** → **265120**.

**Named state / facts:**
- **Stella's cast:** there is no separate spell-select step (Z showed no state change and she seems to have the one spell). Hold X for about 95 frames until the magic circle is complete, then release.
  - `lpose` = **20112** is Fire Wall (it stays 20112 for the whole cast).
  - **Fire Wall costs 3 MP** (`mp` 51 → 48). The HUD shows "Fire Wall (MP:3)".
  - It did 21 damage to a Blue Slime.
  - X tapped = dagger: `lpose` read 10303 just as the cast began.
- **310130 hall layout:** zones packed as id/x/y (pixels):
  - Upper floor (y 224): id3 640 → upper floor of 310120; id6 1088 → 310140 Stella's room; id9 1504 → 310150 father's room (it has a "Another portrait of Mother..." line).
  - Middle floor (y 480): id2 640, id5 1088, id8 1536.
  - Ground floor (y 736): id1 672 → ground floor of 310120; id4 992; id7 1408.
  - Stairs: upper → middle at the far LEFT, middle → ground at the far RIGHT.
- **310120 zones:** id4 (76800, 22400) upper floor; id3 (76800, 48000) middle floor; id1 (25600, 73600) → garden; id2 (67200, 73600).
  - The dark lattice at x ≈ 126000 on the upper floor is a wall, not stairs.
- **To list zones without comprehensions,** pack each zone into one number per k: `s32(A+0x274)*1e8 + x/100*1e4 + y/100`, where `A = s32(world+0x11e0+4k)`, x = `s32(s32(A+0x40)+4)` and y = `s32(s32(A+0x40)+8)`.
- `foe` = 1 near the slimes.
- `lid` is still 0 for Stella.

**Walkthrough corrections (EN-old):**
- The manor stairs are in the hall 310130, not in 310120: go upper → far-left stairs → middle floor → far-right stairs → ground floor, then take the ground-floor doors to 310120 and the garden.
- The interlude has a Stella combat tutorial (Yes/No) on leaving the manor, which the guide doesn't mention.
- The east gate of 300120 opens after the Rowen scene, as the guide implies. The road rooms are **265110** and **265120**.

**Tool gaps:**
- **Recording a menu pick is a trap:** `tap --text` with gap 10 picked Yes on a menu before I could choose. Several `rewind`s, each a 60 s replay, were needed to record the No pick. A "stop when a choice menu opens" predicate (a named menu cursor) would help.
- **`ram diff` is still swamped by framebuffer noise.** Narrowing it to the leader slot helped.
- **`eval` has no comprehensions,** so zone listing needed 20 long expressions. A built-in `zones` listing (id, kind, x, y) would save many turns.
- **Fight searches were slow** (about 146 ticks/s in 265110).

Not reached: the chest under the ledge, the lake street, the White Rose Inn (save), the vines, the Barness Hills Stella route and the Stella boss. Next leg: `rd new NEXT --from story-si:end`, or from this movie at f99334.

---
model claude-opus-5-5, effort high, 114 turns, $1.89, 25 min

