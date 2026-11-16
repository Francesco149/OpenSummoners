# probe story-herb-boss

run `story-hc2`; accepted 2026-11-16 12:18

check PASS (tools/drive/tasks/story-herb-cave2.md)
verify MATCH (+ real device)
filed tests/scenarios/story-herb-boss/trace.jsonl (271179 frames)

## driver's report

## story-hc2: all 3 torches lit, scales wall opened, crates and tunnel chests looted, MoH 6 taken, Baby Mage defeated (f271059). Stopped just after the boss when the budget ran out.

**Status:** the movie ends at **f271179** in room 320165, 37k frames past the start (f234130). `rd check` **PASSED** at f264855: step 1 at frame 779, step 2 at frame 3001, the forbid never fired. `rd verify` gave **MATCH at f264855** (rng 55cf3b68). **Not re-verified:** the last ~6.3k frames (f264855 → f271179: back through 320158, the boss cutscene and the fight). Run `rd verify story-hc2` before filing.

**End state:** 320165, lx 73016, the boss dead (`bhp` 0), script 0. Arche has 73/188 HP and Stella 63/145.
- **Not seen:** whether a post-boss event fires. 120 idle frames after the kill showed no dialogue. The next driver should look, then find the chest [Topah Herb] and the exit; `rd zones` lists zone 2 ↑ (22400, 38400) → 320170.
- Near the end the session failed with "savestate pool exhausted" (after about 10 checkpoints and many searches). `rd stop` + replay (328 s) cleared it.

### End saves
- **e5446d5a6ef6** (f261786): goddess-statue fountain in 320160, slot 11, before MoH 6.
- **3b7de61d6d91** (f264678): same statue, slot 11 overwritten, **after MoH 6**. Use this one to continue.

### Route (frames)
- **f234408 → 234886:** from the shelf, up-left, down the central shaft and right into the lower-right cave of 320140. The route given in the task is correct.
- **f235532:** killed the 2 Myconids. Sheathed (Z), pushed block 70210 from x 313600 to its stop at **x 374400** (f237175).
- **f238064:** Arche climbed block → torch-2 shelf with air `U X` (rising slash), which gains about 16800 of height. Swapped to Stella (`ent`).
- **f238165: torch 2 lit** by Stella with `C*12 RC*24 C*6 RX`.
- **f241672: torch 3 lit** in the bottom basin after Stella burned the vines. "Machinery activating" → **scales wall 70230 (slot 0) state 0 → 1**.
- **f242300 → 247803:** climbed back out of the basin. Its shaft (x ≈ 200–230k) is a 4-cell-step climb that needs air `U X` jumps; plain jumps cannot do it.
- **f248880:** crate 70303 at x 6400 = **Good Child Drop**.
- **f249193:** zone 2 (35200, 134400) → **320150**. Its zones: 1 → 320140, 2 ↑ (259200, 70400) → 320155, 3 ↑ (41600, 44800) → 320158.
- **f251454:** → **320155**. Chest (52800) = **Revival Drink** (f252572); chest (60800) = **300 CP** (f252742).
- **f255856:** → **320158**, the tunnel: zone 2 walk-in at x 0 → 320165 (the boss room); zone 3 ↑ (297600, 44800) → 320160.
- **f260758:** → **320160**. Its left end has the goddess statue/fountain 70410 (30400); V on it = drink (full heal) + save. Its right end has **MoH 6** 70100 (235200, floating): touched by a jump at **f262786**.
- **f267621:** → 320165, a long Arche/Stella/"????" cutscene, then the fight from f269175.
  - Kobolds (code 51030, 77 + 72 HP) dead by f269927. Baby Mage dead at **f271059**.

### Party at the end
- **Arche:** Lv6, now cap 9 (MoH 6). Iron Sword, Leather Armor.
- **Stella:** Lv5, 145 HP, 74 MP max. Cloth Armor.
- **Money:** 2141 CP; the cap became 10000.
- **Items:** Revival Drink, Magic Candy, Good Child Drop ×1, Guardian Drop, Mystery Juice, Bronze Sword, Green Smock, Underwear. **All Better Herbs and the Honey are used up.**

### Upgrades taken / skipped
- **Taken:** MoH 6. Honey used on Arche (51 → 188).
- **Kept:** Good Child Drop. The inventory shows only ×1 after the crate, though the crate said "You found Good Child Drop". I kept it for Sana, the main permanent caster; Stella leaves after this chapter until Day 8.
- No new gear was found.

### Named state (this run)
- `shp`: Stella's HP.
- `h0`..`h9`, `h13`: per-slot hostile HP, counted when the slot is in use and has faction 2.
- `bx`: x of 320140 slot 1, block 70210.
- `t2` / `t3`: state of 320140 slot 4 / slot 3, torch 2 / torch 3 (1 unlit, 0 lit).
- `wall`: state of 320140 slot 0, the scales wall (1 = open).
- `bhp`: HP of slot 3 = the Baby Mage in 320165.
- `bxx`: the boss's x.
- These read room slots, so `bx` / `t2` / `t3` / `wall` mean other things in other rooms.

### Game facts / walkthrough corrections
- **Baby Mage** (code 100170) has **297 max HP**, not the guide's 400; it heals when low. The 2 adds are code 51030 = Kobolds. In the boss room, character slots 0–2 hold the party (50010, 50012, 50013) and the boss is slot 3.
- **The fountain is a goddess statue** at the left end of 320160, the room right of the tunnel, together with MoH 6 at the right end. The tunnel's left end (320158 → 320165) is the boss, not the fountain.
- **F2 quick-heal** is a ~200-frame bar while enemies are near and instant otherwise. It picks the most hurt member.
- **Honey** heals 180–240. **Revival Drink** gives 120–160 HP to a downed ally.
- Character position = `s32(s32(c+0x40)+4 / +8)`.
- In 320140, `rd look` often draws only the top strip, with black below. That is a rendering artefact, not void.

### Tool gaps
- Stop conditions are limited to 598 tokens, so I could not define a "hostiles near the leader" value; `actors()` is not mapped for this game.
- The savestate pool ran out after many searches and checkpoints. There is no `ck delete`; `rd stop` + a 328 s replay was the only fix.
- Searches with score and `--apply-best` sometimes produced 3–4.5k-frame detours, and refine did not shorten them.
- `rd map` has no zoom/crop: the 128-cell room is downscaled, which makes ledge widths hard to read.

---
model claude-opus-5-5, effort high, 244 turns, $9.13, 120 min

