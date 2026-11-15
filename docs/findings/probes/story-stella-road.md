# probe story-stella-road

run `story-sr`; accepted 2026-11-15 13:18

check PASS (tools/drive/tasks/story-stella-road.md)
verify MATCH (+ real device)
filed tests/scenarios/story-stella-road/trace.jsonl (121273 frames)

## driver's report

## story-sr: Stella reached the White Rose Inn and saved there; the run stopped at frame 121273 in tunnel 260165, before Barness

**Status:** unfinished. I did not run `rd check` or `rd verify` before the budget ran out, so the trace is neither checked nor verified. By reasoning only, task step 1 should hold at f99334 and step 2 from about f101118, and no title return happened. The movie ends at f121273 (about 22k frames past the f99334 start). Stella is solo in **260165**, Lv3, HP 107/107, MP 3/53, 913 CP.

**End-save digest:** `2f229790ba58`, from `rd endsave` at f108634. The game save is slot 8, "White Rose Inn, Stella LV2". Continue with `rd new NEXT --from save:2f229790ba58`.

**Checkpoints:** start, chest1, top, r265130, r265140, r265150, r265160, inn, innsaved, r265170, r260190, r260180, r260150, r260160, r260164, r260165.

**Route reached (frames):**
- **265120:** a water pit (−4 HP, back to the ledge). The exit is at the top right (zone 2, x 201600, y 12800), reached by climbing from the chest ledge. → **265130** at f101118.
- **265130 has two exits:**
  - The signed wooden gate (zone 2, x 44800) leads to the Loch Carudale side. That is **265180 → 265190 → 265200** (Merkids, a "Loch Carudale" sign, a boat, deep water). The only door there (zone 2, x 64000, by a guard) goes into **330140**, a cave holding about 7 Lead Merkids at 111–126 HP. This side is a dead end for Lv2 Stella; I backed out.
  - **The right exit (zone 3, x 92800, y 25600) is the real road:** 265140 (f102796) → 265150 (f105067) → 265160 (f106700).
- **White Rose Inn:** its door is zone 3 in 265160 (x 99200) → **265165** (f107787).
  - Talking to the woman at the right counter offers a 7 CP rest. My `ent` mashing picked Rest. After the night she offers to save.
  - Saved to slot 8 at f108588.
- **Vines:** east of the inn in 265160 (x ≈ 205000) are flowered vines with a chest behind. Two Fire Walls on them did NOT burn them; maybe these vines don't burn, or they need more hits.
- **Road on:** the 265160 exit is zone 2 (x 252800, y 19200) → **265170** (f111934) → **260190** (f114711).
- **260190 sign:** ↑ Weathervane Tower & Loch Carudale, → Barness & Tonkiness, ← Chartreux. → **260180** (f115193) → **260150** (f116722).
- **260150 sign:** Barness Village / Town of Tonkiness / Weathervane Tower & Chartreux. Its gate (zone 2, x ≈ 53000, ↑) → **260160** (f117117).
- **260160:** the top-left exit (zone 2, x 0, y 12800) is behind a wall too tall for Stella's jump. The bottom cave (zone 3, x 86400, y 54400) → tunnel **260164** (f118790; Stella reached Lv3 here).
- **260164:** its wooden door (zone 2, x 131200) → **260165** (f119937).

**Where it stands:** 260165 is a stone room with a hanging metal weight on a chain at x ≈ 96000. Its only door is the way back (zone 1). Stella stands on top of the weight (ly 29600). Walking left from there and pressing V at the weight did nothing. The way on was not found.

One search claimed it found a new room from 260165, but replaying its printed path went back through the door into 260164 instead. Either the search diverged from the live session or the printed path is wrong; that needs checking.

Not reached: Barness, the man by the cave, the Barness Hills Stella route (torches, vines, clover lever, Wind Crest) and the Stella boss.

**Walkthrough corrections (EN-old):**
- The road east of Chartreux is 265120 → 265130 → 265140 → 265150 → 265160 (inn) → 265170 → 260190 → 260180 → 260150 → 260160, with more rooms after.
- The "optional cave w/ Lead Merkid" is off the 265130 signed gate, via the Loch Carudale rooms 265180–265200. It is not on the main road.
- Deep water is a pit for Stella too: she sinks, takes −4 HP and is put back on the ledge.
- Fire Wall hits about ±17000 lx on BOTH sides of Stella, not on her own spot.
- The `F` key healed instantly twice (HP to full), then did nothing. Herb / Better Herb used from F1 → Items healed at once, not through a field cast bar.
- The inn's rest prompt takes `ent` as Rest.

**Named state** (defined in this run):
- `e0hp`, `e1hp`, `e2hp`, `e4hp`: HP of enemy slots 0, 1, 2 and 4.
- `ehp`: sum of slots 0–5. `e345`, `e5` (slots 0–4) and `eall`: sums used for fight searches.
- Enemy codes: 51011 Blue Slime, 51012 Yellow Slime, 51025 Mothbee, 51040 Merkid, 51041 Merkid Mage, 51042 Lead Merkid, 51051 Viper; 50012 is Stella.
- `zunder` = 101 is a sign's examine zone.

**Tool gaps:**
- My first `-j 4` search crashed the session, and the rebuild replays all ~100k frames (80–100 s each, also after `rewind` and loading a dropped checkpoint).
- There is no zone or enemy listing: `actors()` is unmapped and a stop condition is limited to 598 tokens, so a "near enemies" predicate cannot be written. `zrest` stood in for it.
- Search paths printed as "found / not applied" did not replay the same way.
- `control` stays true while the F1 menu is open.
- Fight searches run at about 150–300 ticks/s.

---
model claude-opus-5-5, effort high, 300 turns, $8.11, 149 min

