# probe story-herb-cave

run `story-hc`; accepted 2026-11-16 06:51

check PASS (tools/drive/tasks/story-herb-cave.md)
verify MATCH (+ real device)
filed tests/scenarios/story-herb-cave/trace.jsonl (234130 frames)

## driver's report

## story-hc: through 320120 → 320130 (crescent wall opened, Stronggirl Juice) into the big torch room 320140. Lit torch 1 (top right). Stopped there: I could not find a way to torch 2. Baby Mage not reached.

**Status:** `rd check` PASSES (step 1 at frame 779, step 2 at frame 3001, the forbid never fired). `rd verify` gave **MATCH at f234130** (rng 4a8b22b3). That is the end of the movie, 14.9k frames past the start (f219262). **No end-save digest:** I passed no statue, fountain or inn, so the last save is still the Tonkiness slot 10, `9ebc4f357286`.

**End state:** room 320140, lx 302468, ly 120100, on the middle floor left of the torch-1 ledge. Arche has 123/188 HP and Stella 145/145. Torch 1 is lit and torch 3 is not. The nearby slimes and myconids are dead.

### Route (frames)
- **f219262:** start in 320120.
  - The status line at frame 0 showed the movie-end values (lx 67208, HP 115). One frame later the live values were lx 80340, HP 106.
- **f220375:** killed the Yellow Slime holding zone 3. A zone does not fire while a hostile is within range.
- **f220703:** 320120 zone 3 (x 0, y 96000, walk-in) → **320125**. This is the Memory Photo 4 side room (no exit forward):
  - It has a vine clump (code 70400 ×19, around x 57–89k), a chest 70202 inside it at (72400, 35200), and a chest 70220 at (8000, 38400).
  - The wooden beam is not solid. Walking off the ledge at x ≈ 130000 drops you into a pit (−9 HP, back to the ledge).
  - **I skipped the photo.**
- **f221321:** used a Better Herb on Arche (62 → 185).
- **f221840:** back via 320120 zone 2 (x 28800, y 121600, ↑) → **320130**. Its zones:
  - zone 1 → 320120;
  - zone 2 ↑ (304000, 51200) → 320135, the 💎 tunnel;
  - zone 3 ↑ (387200, 57600) → 320140.
- **f223876–224200:** Stella burned the vines in 320130: 6 of 7 pieces at x 26800–31600. The top piece (31600, 47600) does not block the path.
- **f224443–224833:** Arche, sword sheathed, jumped over the stone block (code 70210, x 16000) and pushed it right onto the blue button (70215, x 45600).
  - The button state goes 0 → 1 and the **crescent wall 70230 at x 354200 (handle 320130501)** goes 0 → 1. **This wall is on the far right of the room, not above the button.**
- **f228037:** → **320135**. Fight with 2 Kobolds and a Myconid, then the chest (22400, 38400) = **Strongirl Juice** (f228719).
- **f229663:** 320130 zone 3 → **320140**, the big area.
- **f232502:** **torch 1 lit** (70251 at 368000, 102400, handle 320140501).
  - I first lit torch 3 too (70251 at 57600, 230400, handle 320140503) after burning the bottom vines. That branch was cut when I reloaded to explore; it took about 240 frames once Stella was in the bottom basin.

### Where it is stuck: torch 2
Torch 2 is 70251 at (393600, 160000), handle 320140502. The block 70210 at (313600, 188800) and two Myconids at (345000, 188800) sit in a lower-right cave, about x 270–400k and y 142–200k. I could not find the way in:
- From the torch-1 ledge, the floor ends at x ≈ 379000.
- The middle floor has nothing below it (black, checked with `pd`).
- The bottom-left basin ends at an invisible wall at **x 231600, y 196000**: ↑, push and slide did not get through.
- About 8 searches, totalling over 1500 s, never got lx > 232000 below y 170000.
- Zone 2 (35200, 134400) → 320150 cannot be reached before the scales wall opens (searched from the entrance).

The guide's "via pushed block right of lower tunnel" does not match any passage I found. Next driver: map 320140's lower-right cave first, maybe via a hidden passage or a breakable wall.

### Party at the end
- **Arche:** Lv6 (cap 7), 188 max HP (181 base + 7). Iron Sword, Leather Armor. Armor #2 and Accessory are still empty: nothing found fits.
- **Stella:** **Lv5** (she levelled from 4 to 5 near the run's start), 145 max HP, 2 MP. Cloth Armor.
- **Money:** 1504 CP.
- **Items:** 3 Better Herbs, Magic Candy, Honey, Good Child Drop, Guardian Drop, Mystery Juice, Underwear, Bronze Sword, Green Smock.

### Upgrades taken and skipped
- **Taken:** Strongirl Juice → **Arche**, Max HP +3. Its text: "permanently increases Max HP". Arche is the leader and tank and took most of the hits.
- **Kept:** Good Child Drop for Sana, the main caster; the Guardian Drop and the Mystery Juice too.
- **Skipped:**
  - Memory Photo 4: no route found to the vines without falling into the pit.
  - The 320120 chest 70220 at (177600, 60800): the guide's poison chest.
- **Not reached:** MoH 6, the fountain and the Baby Mage.

### Game facts and walkthrough corrections (EN-old)
- **Stella's Fire Handling** comes out on **↓+X**; L+X did a Stab. Search found the air shot `C*6 LX` works.
  - Each vine piece is a separate 70400 object (state 0 → 1 when burnt).
  - Braziers are code 70251, state **1 = unlit, 0 = lit**.
- **Crescent puzzle (320130):** the burned vines guard the block. Push the block from x 16000 to the button at 45600. That opens the crescent wall at x 354200, before zone 3, not at the button.
- The 💎 tunnel 320135 (zone 2 at x 304000) is **east of the button**, not "before the crescent".
- **Menus:** with an enemy near, an item used from F1 becomes a timed cast. A cast on Stella (~280 frames) did not save her at 6 HP; she died under Myconid hits. Clear nearby enemies first. With no enemy near, it is instant.
- F1 first shows any pending "Level-up Info" screen.
- **Better Herb** healed Stella +113 (to full).
- **Myconids** (spore clouds) and Kobolds in 320130 / 320135 / 320140 hit Stella's AI hard. She dropped to 0 HP in one search; keep an `shp` avoid in every search.

### Named state (defined in this run)
- `ehp`: HP sum of hostile slots 0–9.
- `e0`..`e7`: packed `hp*1e10 + (x/100)*1e5 + y/100` per hostile slot. **Test `e // 10**10`, not `e == 0`.**
- `shp` = Stella's HP (party slot 2: `s32(s32(s32(world+0x2784)+0x4038)+0xa64)`); `ahp` = Arche's HP (party slot 0).
- Object state = `s32(s32(world+0x11e0+4k)+0xc)`. The k values are 320140 zone-list slots; in 320140, torch 1 = k2, torch 3 = k3, torch 2 = k4.
- Stella as leader reads `lid` 0.
- **Not named:** the follow mode and the Baby Mage's HP (not reached).

### Tool gaps
- **No room map or overview.** Black void cannot be told apart from walls, and this blocked the torch-2 puzzle. A collision or tile dump per room would have solved it.
- **No tuple or list results in eval,** so I had to pack numbers by hand.
- **Each load or rewind of an old checkpoint rebuilds by replay (~290 s).** It cost real time three times.
- **`rd new --from`'s first status shows stale lx / HP.**
- **`--apply-best` hides the path text** (only plain `--apply` prints it), so I could not re-type a branch after a reload.

---
model claude-opus-5-5, effort high, 226 turns, $9.19, 141 min

