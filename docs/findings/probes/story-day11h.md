# probe story-day11h

run `story-d11h`; accepted 2026-11-27 03:30

check PASS (tools/drive/tasks/story-day11h.md)
verify MATCH (+ real device)
filed tests/scenarios/story-day11h/trace.jsonl (29441 frames)

## driver's report

# story-d11h report

The party is out of the Archmage's Tower and saved at the North Chartreux Guardpost inn, but this leg did not reach school, the Labyrinth of Night or MoH 18. Getting out of the tower was the slow part: the Recall Lamp is refused inside it, and the way in can't be walked back. `rd check` PASS and `rd verify` MATCH on bunker.

**Run:** `story-d11h` (from `save:e65ce3a856fa`, `RD_HOST=bunker`). It ends at **f29441** in the Guardpost inn (430120), in free control with Arche leading.

**Gates:**
- `rd check … story-day11h.md`: PASS. Step 1 "in control" at f845; step 2 "left the tower" at f25627.
- `rd verify`: MATCH. Live session and headless replay both end at frame 29441, rng 5ebf2ded, semv fdf166168256e652 (fb 2b89ff4fbd540d33).

**End save: `dfd67b2a393b`**, slot 10, at the Mountain Cat Inn after resting. Slot 10 held "Town of Tonkiness Arche LV6 4:40", the shortest play time outside slots 1–9 and 24. Restart with `rd new NEXT --from save:dfd67b2a393b`.

## Why I stopped here
- About 28.6k of the ~40k frame budget was gone by the time the save was made.
- The route on to school is very long. From the Guardpost it goes 430110 → 420180 → 420170 → 420160 → 420150 → 420120 → 420110 → Chartreux, then the loch and bridge to Tonkiness, then east to school. Those rooms include wide caves with enemies.
- That would not fit in the remaining ~11k frames, and an unsaved partial walk can't be picked up by the next leg. So I saved at the inn and stopped.

## Route
| frame | what |
|---|---|
| 844 | Continue → slot 9 (page 2) → 440220 bedroom, in control |
| 1474 | Recall Lamp in 440220: "For some reason, you cannot teleport from here." The lamp is refused inside the tower. |
| 2459–3904 | Entering 440200 starts a cutscene: Chiffon still can't remember anything; "let's get back to school" |
| 5740 | 440200 bottom-left door → 440190 |
| 9144 | 440190 → 440160, lower floor (the way story-d11d came in) |
| 9374–9929 | Tried to climb from 440160's outside ledge to its upper floor with Unison. It falls about 5000 short (details below). Gave up on that way. |
| 10156–14376 | Back through 440190: up the step at x ~135500, then the right exit (214400, 108800) → **440170** |
| 14819 | Life Dew used from the menu at 440170's top: the whole party back to full HP |
| 15462–16766 | 440170: dropped through the hole; fight search at the bottom-left door → **440150** |
| 16806–20768 | 440150 → 440130. Pink slimes (code 100186, slots 12+) keep holding the door. |
| 22727 | 440130: down the outside platforms to its lower door → 440120 |
| 25192 | 440120 → 440110 |
| **25627** | 440110 zone 1 → **420240, outside the tower door** |
| 26289–26455 | Recall Lamp → **430110, the Guardpost** (its "retry" room) |
| 27600–27812 | Item shop at the left counter of 430120: Better Herb ×5, Life Dew ×2, Recall Lamp ×1 |
| 28627–29441 | Inn, right counter: Rest (28 CP), Save → slot 10 |

## Party at the end
- **Levels:** Arche Lv16 (281/51), Sana Lv16 (212/279), Stella Lv16 (248/228). Sana and Stella levelled up during the escape; all three are at full HP/MP after the inn.
- **Stella learned Interceptor** at Lv16. It arrived switched on.
- **Money:** 19459 CP.
- **Gear:** unchanged. Arche: Org Slayer, Scale Armor, Leather Cape, Kitty Ears. Sana: Rune Staff, Scale Armor, Chest Sash, Green Smock, Anti-Poison Ring. Stella: Lady's Dagger, Scale Armor, Woven Gloves, Magic Bracelet.
- **Items:** Better Herb ×11, Best Herb ×4, Life Dew ×2, Revival Drink ×3, Magic Candy ×3, Recall Lamp ×1, Venom-Away S ×3, Sweet Sticks.

## Purchases, and upgrades skipped
- **The Better Herb buy was an accident.** Mashing `ent` into the Old Lady's shop bought 5 Better Herbs at 18 CP each. It happens to meet the "~10 Better Herb" target.
- **Bought on purpose:** Life Dew ×2 (96 each), Recall Lamp (500), and the inn (28).
- **Not bought:** Revival Drinks (already have 3), Orange Smock (125, not clearly better than the Green Smock), Magic Bracelet (Stella already wears one).
- **Weapon / armour upgrades:** none. The Guardpost has no weapon or armour shop. Tonkiness and Chartreux shops were not reached.

## Walkthrough corrections (EN-old, Day 11 after Unison)
- **The tower must be walked out; the lamp only works outside.** Inside the Archmage's Tower the Recall Lamp says it can't teleport. At 420240, just outside, it works and goes to 430110, the Guardpost (`retry` = 430110), not to town.
- **The way in is one-way.** story-d11d's drop off 440160's upper floor can't be climbed back, even with Unison.
  - A full triple jump (`LC*6 C*34 . RC*40 . RC*40`) starts from the 440160 lower ledge at ly 84000.
  - Each jump rises about 15500, peaking at ly ≈ 37900 against the wall at lx 23600.
  - The upper floor's lip is still about 5000 above Arche's feet there.
- **Exit route, entry to exit:**
  - 440200 bottom-left door (stand at lx ≈ 27200; `zones` x is the left edge) → 440190.
  - 440190: bottom corridor right; jump the step at x ~135500; right exit (214400, 108800), which needs ↑ → 440170.
  - 440170: drop through the middle hole; bottom-left ↑ (16000, 73600) → 440150.
  - 440150 zone 1 (198400, 48000) → 440130 upper floor.
  - 440130: drop down the outside platforms to the lower door (137600, 99200) → 440120.
  - 440120 zone 1 (268800, 35200) → 440110.
  - 440110 zone 1 (67200, 35200) → 420240.
- **Leaving 440210 starts a cutscene in 440200**, about 1500 frames, which ends with "let's get back to school".
- **Unison:** Arche drifts during the cast, about 3000 lx over ~200 frames. On a ledge edge she falls off; cast away from edges.
- **Guardpost 430120:** the left-counter "Old Lady" is the item shop (Buy/Sell; Better Herb 18, Venom-Away S 40, Life Dew 96, Revival Drink 350, Magic Candy 120, Recall Lamp 500, Magic Bracelet 11000, Orange Smock 125). The right-counter "Old Man" is the inn (28 CP; Rest heals, then a "Want to save?" prompt).
- **Life Dew used from the menu out of combat is instant**, with one message box per member. It is not the timed cast bar a field heal becomes in a fight.

## Named state (run-local)
- **Party HP:** `phmin` and `m0`/`m1`/`m2`, read from `s32(s32(s32(world+0x2784)+0x4030+4*i)+0xa64)`.
- **Hostile HP per character slot:** `h0`…`h19` = `s32(s32(c+0x750)+0x54) if u8(s32(c+0x40)) and s32(c+0x1dc) == 2 else 0`.
- **Enemy position** is `s32(s32(c+0x40)+4)`; `s32(c+4)` is wrong.
- **Party slot numbering moves:** in 440150 the party sat at slots 8–10 and spawned Pink Slimes (code 100186) at 12+. Check before scoring.

## Tool gaps
- **Fight searches are slow:** 74–280 ticks/s with many enemies in a room.
- **The 598-token limit allows only about 4 proximity-filtered enemy-HP terms.** An `actors()` helper, which `rd` doesn't map yet, would make fight scores much easier.
- **`rd look` sometimes names its file one frame off** from the frame `play` reports (f15441 vs f15440). Use `ls -t` to find the PNG.
- **Death in the 440170 fight:** in the first attempts Stella died twice. 4–5 enemies (Harpy, Org Swordmaster, Pink Slimes, Kobold) were clustered at 440170's bottom door while she was below 180 HP. I reloaded both times; the kept path heals to full before dropping in.
