# probe story-day7c

run `story-d7c`; accepted 2026-11-17 23:28

check PASS (tools/drive/tasks/story-day7c.md)
verify MATCH (+ real device)
filed tests/scenarios/story-day7c/trace.jsonl (380399 frames)

## driver's report

# story-d7c: Carudale Tunnels to Chartreux City (Day 7)

**Run:** `story-d7c`, started from `story-d7b` at f339205 and ending at **f380399** (41.2k frames). Two drivers played it:
- An API driver played f339205–360654 (212 turns, $5.82). It died when API credit ran out and wrote no report. I recovered its part from its log.jsonl.
- A Claude Code subagent played f360654–380399.

**Gates:** `rd check story-d7c tools/drive/tasks/story-day7c.md` **PASS**. `RD_HOST=bunker rd verify story-d7c` **MATCH** (f380399, rng 5c18ef75, semv dbe5eb6e63c2efa6, fb 1e83e2cea8bd0ea1). Nothing touched Windows.

**End save:** Dancing Piglet Inn, Chartreux, written to slot 12 (page 2) at f380194. `rd endsave` → **`1e239cb938ff`** (f380237). The next leg can start with `rd new NEXT --from save:1e239cb938ff`. This is the first save since the Tonkiness save d89ccc7f7e38.

**End state:** f380399, room 300240 (inn, 2F), control. Gold 2503 CP, `story` = 1210, both members at full HP.

## Route steps reached
- f339205: start in 330120. Killed the Merkid-class foes, then ↑ zone 3 → 330125 at f340144.
- f342145: → 330126.
- **f342358: MoH 7** (330126, lx ≈ 38880, C*14 jump).
- **f343451: lever** (330126, lx ≈ 69600): "machinery activating".
- f346824: back via 330125 / 330120 → 330128.
- f349221: → 330130.
- **f352994 / f353138:** two chests in 330130's bottom-left (lx ≈ 32000, ly 68000): **[Better Herb] (stack of 3)** and **[Magic Candy]**.
- f354471: → 330135. f355631–358729: pushed the 3 tall platforms (`tA`/`tB`/`tC`) into place and crossed.
- f360614: → 330140, the lake. f361127: Heal Water on Arche, 42 → 197 (Lv7 now).
- f361639: Sana reached chest 603's rock. Not opened: a Lead Merkid stood next to it.
- **f362066: Sana stood on the sunken boat** (70600 at (196800, 108800)), and it rose to the surface by f362366.
- f363783: the boat reached the right ledge with Sana facing right, and **Arche boarded**. f365183: the boat crossed to its left stop with Sana facing left.
- f367564: killed the Merkid Mage, then the Lead Merkid, then ↑ (41600, 32000) → **265200** (outside).
- f368338: Arche, sheathed, pushed the boat (70600) right into the water.
- **f369480 / f369635: island chests [Mystery Juice] and [Magic Candy].**
- f370539: → 265190. f372647: killed the Merkid at the exit, ↑ at (0, 16800) → 265180.
- f373545 → 265130, f374346 → 265120, f375046 → 265110.
- **f376492: → 300120, Chartreux City.** The arrival talk (Colm, Chupil, Leon: "not many sweet sticks left") ended with control at f378293.
- f378914: inn 300240.
- f379476–380237: rest 8 CP, then save to slot 12.

## Party at the end
- **Arche:** Lv7, 197 HP, 31 MP, Marks 7. New ability: Combo Wide Slash. ATK 36, DEF 31, Spirit 18, Resist 23. Gear: Iron Sword + Leather Armor. Armor #2 and Accessory empty.
- **Sana:** Lv5, 104 HP, 100 MP. Gear: Iron Staff + Cloth Armor + Green Smock.
- **Items:** Herb ×1, Better Herb ×3, Magic Candy ×3, Guardian Drop, Mystery Juice, Underwear. No Revival Drink.

## Upgrades: taken and skipped
- **Taken:** MoH 7, Better Herb ×3, Magic Candy ×2, Mystery Juice.
- **Skipped: [Life Dew] and [Cloth Gloves]** (330140 chests 70220 / 330140603 at (321600, 73600) and 330140602 at (147200, 108800)). A Lead Merkid (112 / 130 HP) guards each one. The touch pass refuses with a hostile within 20000 × 16000, and Sana's underwater attacks barely hurt them (112 → 92 in a 150 s search).
- **Not used:** Mystery Juice (Sana), Guardian Drop, Magic Candies.
- **Not done:** the Chartreux weapons store ([Power Drop] drawer, sword / armor) and the item shop (Better Herbs, Revival Drink). The frame budget ran out at the inn. **No shop lists or prices were seen.**
- **Wasted:** 2 Herbs (+4 HP each) from F2 spam in the boat-ride search.

## Named state (run vars only)
ahp, shp, ehp, nhp, slimes, f2hp, m0, m5, mage, e0 (foe HP), tA/tB/tC (330135 platforms), boat/bx (330140 boat state / x), bx2 (265200 boat x), c603/c602 (lake chests), i1/i2 (island chests), ax/ay (Arche's body when not leading).

## Game facts and walkthrough corrections
- In 330140, Sana does not swim. She sinks at about +213 ly/frame, and C does nothing mid-water; only a floor jump (7–12k high) works. Steer while sinking to land on underwater ledges.
- The sunken boat 70600 rises when the leader stands on it. It moves about 200/frame toward the leader's facing and stops at the lake ends. Arche (AI) boards it by herself.
- The Merkid Mage (51041) heals the Lead Merkid (pair HP 75 → 108 mid-fight). Kill it first.
- `rd zones` calls 265190's west exit "walk in", but it fires only on ↑.
- Walkthrough fixes, Day 7 step 4: the "water chests" are guarded by Lead Merkids. The [Better Herb]×3 is one chest (a stack of 3) in 330130.
- Room chain: 330120 → 330125 → 330126 → 330128 → 330130 → 330135 → 330140 → 265200 → 265190 → 265180 → 265130 → 265120 → 265110 → 300120.
- Chartreux inn = 300240: Old Lady at lx ≈ 77000, talk facing left. 8 CP, offers a save after resting.

## Tool gaps
- The bunker session was rebuilt 4–5 times, about 9 minutes each. Each rebuild came on a command that started with `rd ck … drop` ("it had no live savestate").
- The savestate pool ran out for the dead driver.
- The `rd zones` kind label is wrong for 265190.
- Searches with `f2` in the action set burn healing items.

## Repo edits
None. No commits.

