# probe story-stella-barness

run `story-sb`; accepted 2026-11-15 16:34

check PASS (tools/drive/tasks/story-stella-barness.md)
verify MATCH (+ real device)
filed tests/scenarios/story-stella-barness/trace.jsonl (153656 frames)

## driver's report

## story-sb: Stella got from the weight cave through Barness into the Barness Hills Dungeon (Stella route) and stopped in 290230 at frame 153656, before the clover lever and the Stella boss

**Status:** `rd check` PASSES: step 1 at f779, step 2 at f3001, and the forbid never fired. `rd verify` MATCHES (f153656, rng 8d7af176). The movie runs about 32.4k frames past the start at f121273.

**Where it stands:** Stella is in **290230** at lx 183584, ly 39200, with HP 30/107 and MP 29/53. She has 973 CP, 1 Herb and 1 Better Herb. Beside her are two chests, two Cocorats and a Killer Bat. I stopped here because HP was low and the fights were costly. A fight search from here only got HP down to 16, so I loaded the `chests` checkpoint back.

**End-save digest:** `0ef7e721d142`, from `rd endsave` at f132258. It is the Weathervane Inn in Barness, slot 9 "Barness Village, Stella LV3", saved right after resting (HP and MP full). Continue with `rd new NEXT --from save:0ef7e721d142`. There was no save point after that.

**Checkpoints:** start, r260160, bees, right1, r260170, r270110, innsaved, r280115, door280160, r280160, healed, r280170, r290110, r290200, vine1, pit1, torches, r290210, door2, r290230, chests.

### Route reached (frames)
- **f121779:** 260165 → 260164. Walk right to the door at lx ≈ 136800 and press ↑.
- **f123473:** 260164 → 260160, found by search. A Blue Slime blocks the cave slope.
- **f124851:** in 260160, Mothbees put Stella to sleep (pressing keys does not wake her; getting hit does). Two were killed with a fight search.
- **The way past 260160's "too tall wall":** go to the right side of the room (lx ≈ 113000) and run-jump onto the grass ledge (`RC*24 R*16 RC*20`). Climb to the top platform, then run-jump left over the gap onto the wall top. Then ↑ at the top-left exit → **260170** at f126686.
- **f128539:** 260170 → **270110**, the Barness east gate. Scene: "So this is Barness? How very rustic. / Now where is this cave Arche mentioned?"
- **f129954:** ↑ at x 134400 → **270120**, the inn street.
- **Inn:** the door is at lx ≈ 216000 (↑) → **270200**, the "Weathervane Inn", 6 CP a night. The woman's dialogue opened on a V press after the X, Z and ↑ tries; I did not pin down why it fired then. Rested to full (gold 932 → 926). She then offers to save; I saved to slot 9 at f132136.
- **f133592:** the Old Man by the cave (lx ≈ 28000) only gives a flavour line about the wind in the caves.
- **f133908:** the cave at lx ≈ 16000 (↑) → **280115**.
- **f138138:** 280115's door at x 124800 → **280160**, after killing the Yellow Slimes and a Killer Bat that held it. One drop was a Better Herb.
- **f140855:** 280160 → **280170**.
- **f141057:** ↑ at lx ≈ 56800 by the altar → **290110**. Stella says "A man-made dungeon... A Crest of the Wind must be here."
- **f142329:** walk right past the stairs and a Cocorat; ↑ at the right end (lx 202800) → **290200**.
- **f143751:** the dead vines in 290200 (lx ≈ 31000–40000) burn from ↑+X, the upward fire shot.
- **f145209:** 290200 has spike pits (−5 HP, back to the ledge). Search crossed them → **290205**, the torch room.
- **f146040:** all 4 braziers lit (one was already burning), at lx ≈ 63000, 74300 (lit), 85500 and 97600. Walking right then crossed into **290210** at f146222.
- **f152159:** in 290210, the wooden door at lx ≈ 178000 (floor ly 71200) is held by Yellow Slimes; after a Fire Wall fight, ↑ → **290230** (arrival lx 19200).
- **f153656:** 290230, the two chests at lx ≈ 175000–180000. This is probably the guide's "[Better Herb]×3, [Magic Candy]" room, but that is not confirmed.

**Not reached:** the poison chest, spike trap and optional vine chests; the stairs down, Moldy Bread and the clover lever; the talking doors and the Wind Crest; the return to Arche and the Stella boss.

### Game facts and walkthrough corrections (EN-old)
- **Stella has two attack modes, toggled with Z.** The `sword` variable stays 0 for Stella, so it does not show which mode she is in.
  - Dagger mode: X stabs, and a direction + X is a dagger dash.
  - Fire mode: →+X is a long low flame, ↓+X a short flame just in front, ↑+X a diagonal upward shot.
  - Holding X about 95 frames casts Fire Wall in either mode.
- **Dead vines burn only from ↑+X**, which hits their upper part. The low →+X flame passes under them, and a Fire Wall on them did nothing. This probably explains why the 265160 vines "didn't burn" for the last driver.
- **Torches / braziers light from ↓+X** (short flame) or →+X in fire mode.
- **The 260160 top-left exit is not blocked.** You reach it over the top platform from the room's right side.
- **Barness's inn is the "Weathervane Inn" in 270200** (6 CP; Rest, then a save offer). The man by the cave only gives flavour text and is not needed.
- **The Stella route goes:** 290110 right end ↑ → 290200 (vines, spike pits) → 290205 (4 braziers) → 290210 → wooden door → 290230.
- **The `F` key did nothing.** Better Herb from F1 → Items was a cast bar with enemies near (f139659). With no enemies near it healed instantly (+80 HP, f146387).
- **Mothbees put Stella to sleep** (lpose 117). Mashing keys does not wake her; being hit does.

### Named state (this run)
- `e0`–`e5`: HP of enemy slots 0–5, counting only slots in use with faction 2.
- `ehp`: their sum.
- Enemy codes: 51061 Killer Bat, 51071 Cocorat, 51012 Yellow Slime, 51025 Mothbee, 51011 Blue Slime.

### Tool gaps
- **There is no variable for Stella's weapon mode.** The mode is invisible to predicates.
- **`--apply-best` applies long, meandering paths.** Several took 1500–3400 frames and lost HP. A "shortest path to the best cell" refine would help.
- **Fight searches run at 200–350 ticks/s.**
- **There is no zone listing (positions of a room's doors).** I located exits from pictures.
- **Object flags (torches, vines) were not found.** A `ram diff` limited to 0x400000–0x2000000 showed only the RNG; they must live in heap memory, so no torch or vine predicate was named.
- **Search goals keyed on enemy HP are fooled by a room change** (the slots reset to 0). Add `room == X` to such goals.

---
model claude-opus-5-5, effort high, 266 turns, $5.59, 102 min

