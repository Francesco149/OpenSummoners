# probe story-barness-dungeon3

run `story-bd3`; accepted 2026-11-15 08:13

check PASS (tools/drive/tasks/story-barness-dungeon3.md)
verify MATCH (+ real device)
filed tests/scenarios/story-barness-dungeon3/trace.jsonl (89312 frames)

## driver's report

## story-bd3: I got through the gargoyle door and into the Stella interlude (Rowen and the portrait scene), but stopped inside Mayberk Manor at frame 89312

**Status:** `rd check` PASSES (step 1 at f779, step 2 at f3001, the forbid never fired) and `rd verify` gives MATCH. The run ends in the manor at f89312, short of the ~100k mark. I stopped there because my budget was nearly gone and I could not find the way back out of the manor (details under "Where it stopped").

**End-save digest:** `944f0d1ef279` (`rd new NEXT --from save:944f0d1ef279`). It comes from the story save prompt after the gargoyle door, at about f72534. `rd endsave` gave the same digest at f73866 and again at f89312, because no later save was made.
- I advanced that prompt by tapping `ent`, so I don't know which slot it went to.
- I passed no goddess statue in this stretch.

**Checkpoints:** `start` 65958, `killed` 66986, `r155` 67250, `q` 70250, `answered` 71103, `stella` 73866, `manor` 83666, `inmanor` 85121, `rowen` 86832, `room` 87611, `portrait` 88966.

**Route reached (frames):**
- **f66325:** Kobold (51030) and Killer Bat (51061) by the 290150 shaft killed with a `--hold`-less fight search.
- **f66676:** a Yellow Slime killed as well.
- **f67211:** rode the weight and went through zone 2 into **290155**. The weight is actor `world+0x1250` (x 208000); its y moves 100 per frame and bottoms out near 80000.
- **f68308:** killed 2 Kobolds and 2 Yellow Slimes in 290155. HP went 167 → 90, then F2 Better Herb.
- **f68857:** 290155 zone 2 (x 201600, y 12800, top right) → **290160**, the room with the fancy doors and two gargoyles.
- **f69277:** pressed ↑ at the doors, which starts "Mister Mysteriousvoice" and a Yes/No prompt.
- **f70334:** answered 8 once on purpose. Result: "INCORRECT! TO THE ENTRANCE WITH THEE!" sends you to **290110** with HP kept (coverage). I then loaded `q`.
- **f70342:** answered **12** (choice list 8/10/12/14, cursor `D D`). Door opened at f71103.
- **f71104:** **290170**: Stella encounter, then the save prompt (Yes) and the slot list.
- **f72794:** interlude starts with Stella solo in **300120**, Chartreux (`party` = 4, HP 81/81, MP 51/51). Monologue ends at f73866.
- **f83666:** the 300130 stone gate at x ≈ 147000 (↑) → **310110**, the manor garden.
- **f85079:** **310120**, manor interior. Rowen scene at f85274–86832 (Rowen and Mary; the crest is in the Wind Hill caves).
- **f86833:** **310140**, Stella's room: "book" monologue.
- **f87841:** portrait scene ("Mother..."), done at f88966.
- **f89097:** door → **310130**, the hall. f89272: back to the upper floor of 310120.

**Where it stopped:** I could not find the way down from the manor's upper floor to 310110. A 120 s search between 310120 and 310130 failed. Next step: find the manor stairs, then head east to Barness / the White Rose Inn.

**Chartreux layout (my findings, not in the guide):**
- **300120** (east district, arrival):
  - zone 1 at x 0 (up the stairs) → 300110.
  - Doors: x 48000 Inn, 102400 → item store 300250, 134400 → clothes store 300260, 220800 Foods store.
  - Zone 6 at x 281600 is the city gate. Stella refuses it with "I should hurry back to the mansion and ask Rowen".
- **300110** (west):
  - x 35200: gate that refuses the same way.
  - x 80000: candy store.
  - x 126400 (↑): stone archway → **300130**.
  - x 163200 → 300230, the building next to the weapons store.
  - x 220800: ARMS (weapons store).
  - x 380800 → 300120.
- **300130** (north): x 9000 → 300300, a house; **x ≈ 147000 guarded stone gate → manor 310110**; its right edge leads back to 300110.

**Named state (defined in this run):**
- `pair`, `slime`, `foes`: enemy HP sums, `s32(s32(c+0x750)+0x54)` × targetable for fixed slots. They fell to 0 on the kills.
- `wy` = `s32(s32(s32(world+0x1250)+0x40)+8)`, the 290150 weight's y. Validated with `--watch`.
- **HP / MP / gold:** `hp`, `mp`, `gold` from vars.json read the leader's party slot.
  - In the interlude `hp` showed Stella's 81 and `mp` 51, matching the HUD.
  - `lslot` = party slot 2 there.
  - **`lid` reads 0 for Stella** (slot+4 is 0), not an id like Arche's 50010.
- **Party:** `party` = 4 during the interlude (only slot 2 active; Arche's slot `+0x9c4` = 2), 1 before it.
- **Day:** there is no day counter. `story` stayed **1120** through the whole interlude (scripts test 100100101 == 1120 for the Rowen talk, chapter_scripts.c:32964).
- Zone actors (`+0x274` id, `+0x278` kind) hold **no destination room**. Routing is in the room data or scripts, so a door has to be tried to learn where it goes.

**Game facts:**
- The gargoyle question is a two-step: Yes/No, then "Four statues lay beyond this door. Now – how many in total?" with choices 8/10/12/14.
- The interlude opens with "Several hours before Arche met Stella…" and starts Stella at 300120.

**Walkthrough corrections (EN-old):**
- 8.5 "fancy doors" = room **290160**, reached from 290155's zone 2 at the top right.
- The guide doesn't say where the manor is: it is behind the guarded stone gate in the north district 300130 (via the 300110 archway at x 126400). The other two city gates refuse.
- The portrait scene fires by walking right in 310140 (x ≈ 73800), as the guide says.

**Tool gaps / issues:**
- **A live session crashed during a `-j 4` search** at f89312 in 310120 ("the game exited or crashed"). Log: `C:\oss-rhost\runs\drive\story-bd3-599626-719436733\rhost.log`.
- **`rd verify` reported session drift before its MATCH.** The live session and the headless replay had equal frame and RNG but different `semv` (game state); a clean rebuild then gave MATCH. The drift dump showed 11 vs 8 memory ranges and 0 differing dwords. It is kept in `runs/story-bd3/verify-drift-f89312` and should be reported.
- **Can't sum over enemy slots in one stop condition:** status / stop conditions are capped at 598 tokens, comprehensions are not accepted, and `actors()` is unmapped.
- **No list of zones with their positions or destinations,** so finding doors in towns was slow trial and error.
- `ram diff` is swamped by framebuffer and particle noise.
- Searches in town and manor rooms ran slowly (100–300 ticks/s).
- The `ram dwords` VA argument does not accept an expression.

---
model claude-opus-5-5, effort high, 222 turns, $5.78, 71 min


## orchestrator follow-up (2026-11-15)

- verify drift (11 vs 8 ranges, 0 differing dwords) = rh_snap restored pages committed after a savestate as committed
  zeros → fixed in 9862328 (the commit map is savestate; `--snap-test` now checks the restored state itself).
- the `-j 4` search crash: the bunker drops a session's run dir when it ends, so its log was lost; `rd` now keeps a dead
  session's rhost.log in the run dir (`ended-*.log`). Cause unknown — re-check on the next crash.
- `rd verify --real` reported REAL MISMATCH on state_sem (new device-pointer slots in Chartreux / manor rooms, before
  `det --calibrate`): the check is a note now; calibration added 230 slots.
- next leg: `tools/drive/tasks/story-stella-interlude.md`.
