---
name: story-herb-cave2
start: story-herb-cave
steps:
  - loaded into a room with control: world != 0 and control
  - left the start room: room != 0 and frame > 3000
forbid:
  - back at the title after loading: world == 0 and tcount == 5 and frame > 6000
---
**Frontier probe: the Herb Farm Cave's torch room and the Baby Mage, from the end of `story-herb-cave`** (start: that
scenario's movie replayed — f234130, **Arche (Lv6, cap 7, 123/188 HP, Iron Sword + Leather Armor) + Stella (Lv5,
145/145 HP, 2 MP, Cloth Armor)** in **320140** (lx 302468, ly 120100, the middle floor left of the torch-1 ledge);
**torch 1 lit**, torches 2 and 3 not; 3 Better Herbs, Magic Candy, Honey, Good Child Drop, Guardian Drop, Mystery
Juice; 1504 CP; last save = the Tonkiness inn, slot 10, digest `9ebc4f357286`). Read
`docs/findings/probes/story-herb-cave.md` and the walkthrough's [RD story-herb-cave] note first.

**New tools — use them on entering each room:** `rd zones R [--all]` (exits → destination rooms; objects code / state)
and `rd map R` (a PNG of the room's COLLISION grid: grey wall, black open, + zones / objects / party / hostiles — Read
it). The last driver spent ~1500 s of searches on torch 2 because it could not tell walls from void.

**The torch-2 route (from `rd map` of 320140, unverified):** torch 2 (code 70251, (393600, 160000), handle 320140502)
and block 70210 (313600, 188800) sit in the lower-right cave, which opens to the LEFT: from the party's shelf go left
along the y ≈ 120000 corridor to x ≈ 187000–208000, drop down the central shaft to y ≈ 155000–176000, then go right
along that level into the cave. Check this against `rd map` before committing to it. Torch state: 1 = unlit, 0 = lit.
Torch 3 (57600, 230400, bottom basin after burning vines; lit once by the last driver in ~240 frames from the basin).

Next, per the walkthrough (Day 6, step 11.4–11.7): light torches 2 and 3 → the scales wall (left, below the entrance)
opens → crates [Good Child Drop] → tunnel right: [Revival Drink], 300 CP → ledge → tunnel → **MoH 6** (right end) +
the **fountain** (left end: heal + SAVE → `rd endsave`) → down left → **BOSS: Baby Mage** (+2 Kobolds; 400 HP in the
guide; heals at < 60%; `docs/game/bosses.md`) → chest [Topah Herb] → leave → Sana's house cutscene → save (Day 7).

**Upgrades (DRIVER.md "keep the party strong"):** **MoH 6** (cap 9) — Arche is at his cap; take it. Good Child Drop
→ Stella or keep for Sana (say why). Equip anything better a chest gives. Use the fountain before the boss.

1. `rd new <run> --from story-herb-cave` replays that movie into a live session. Confirm where you are: `rd look`,
   `rd status`, `rd map`.
2. Follow the walkthrough's 🔑 steps from there, as far as you can. Save at every fountain / statue / inn and run
   `rd endsave <run>` after each save; note each digest.
3. Fights: `--hold` searches (DRIVER.md); keep an avoid on BOTH characters' HP (`shp` / `ahp` from the last run:
   Stella's HP = `s32(s32(s32(world+0x2784)+0x4038)+0xa64)`). Myconids + Kobolds drop Stella fast. Clear enemies before
   using items (an item with an enemy near = a timed cast).
4. **Run `rd check` and `rd verify` when you reach ~$6 spent**, then go on; verify again before `finish()` if the
   budget allows.
5. Name the state you need (`rd ram R define`): the Baby Mage's HP, the scales wall's state.

Do not grind; do not read other runs' movies. Stop when the run is ~40k frames past its start (the start is at ~234k),
or at a point you cannot get past after real attempts (say what blocks you, with an `rd map` PNG path). Report: run,
frames, the route steps reached with their frames, the end-save digests, each character's level + equipment at the end
and the upgrades taken / skipped (with why), named state, walkthrough corrections, tool gaps.
