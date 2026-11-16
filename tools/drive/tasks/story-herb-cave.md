---
name: story-herb-cave
start: story-herb-farm
steps:
  - loaded into a room with control: world != 0 and control
  - left the start room: room != 0 and frame > 3000
forbid:
  - back at the title after loading: world == 0 and tcount == 5 and frame > 6000
---
**Frontier probe: finish the Herb Farm Cave, from the end of `story-herb-farm`** (start: that scenario's movie replayed
— f219262, **Arche (Lv6, cap 7, HP ~115/185, Iron Sword + Leather Armor, sword drawn) + Stella (Lv4, 126 max HP, Cloth
Armor)** in **320120** of the Herb Farm Cave (lx 67208, ly 134880, low in the room; ~544 enemy HP of slimes still in the
slots); ~5–6 Better Herbs, Magic Candy, Honey, Good Child Drop, Guardian Drop, Mystery Juice; 1363 CP; last save = the
Tonkiness inn, slot 10, digest `9ebc4f357286`). Read `docs/findings/probes/story-herb-farm.md` and the walkthrough's
[RD story-herb-farm] note first: 320120's exits (`rd zones`: zone 1 ↑ (22400, 25600) → 320110 back; zone 2 ↑ (28800, 121600) → 320130; zone 3 walk-in
(0, 96000) → 320125; a chest (code 70220) at (177600, 60800) — the driver misread zone 2's x as 288000); **`rd zones R
[--all]` lists any room's exits + destinations — use it on entering each room**; `ent` in free roam
SWAPS the leader (use `C` for field dialogue); Follow Mode → All Members **Stay There!** is in F1 → Party Commands; the
first R/L press after standing idles ~20 frames.

Next, per the walkthrough (Day 6, step 11.2–11.7): Stella on a platform burns vines → [Memory Photo 4] → the crescent
wall (Stella burns vines, Arche pushes a block onto the blue button under the crescent; 💎 the tunnel before it →
[Stronggirl Juice]) → the big area: light the 3 **scales-symbol torches** (top right; via the pushed block right of the
lower tunnel; bottom after burning vines) → the scales wall (left, below the entrance) → crates [Good Child Drop] →
tunnel right: [Revival Drink], 300 CP → ledge → tunnel → **MoH 6** (right end) + the **fountain** (left end: heal +
SAVE) → down left → **BOSS: Baby Mage** (+2 Kobolds; 400 HP in the guide; heals at < 60%; `docs/game/bosses.md`) → chest
[Topah Herb] → leave → Sana's house cutscene → save (Day 7 begins).

**Upgrades (DRIVER.md "keep the party strong"):**
- **MoH 6** (cap 9) — take it; Arche sits at his cap 7 already, so without MoHs he stops levelling.
- Stat items found here: Good Child Drop → Stella (or keep one for Sana if you can argue it); the found Stronggirl Juice →
  check its text and give it to who it fits.
- Equip anything better that a chest gives. Arche's Armor #2 / Accessory are empty (nothing fit so far) — fill them the
  moment something fits.
- Fountain before the boss: heal + save, then `rd endsave`.

1. `rd new <run> --from story-herb-farm` replays that movie into a live session. Confirm where you are: `rd look`,
   `rd text`, `rd status`.
2. Follow the walkthrough's 🔑 steps from there, as far as you can. Save at every goddess statue / fountain / inn save
   you pass and run `rd endsave <run>` after each save, so later probes can start there; note each digest.
3. Fights: use `--hold` searches (DRIVER.md); keep HP up. Slime-heavy rooms make searches slow — prefer walking past /
   short targeted fights over wide searches. A game over is fine to see once — then `rd undo`.
4. **Run `rd check` and `rd verify` when you reach ~$6 spent**, then go on; anything after the verify, verify again
   before `finish()` if the budget allows.
5. Name the state you need as you go (`rd ram R mark` → act → `rd ram R diff` → `rd ram R define`): the Baby Mage's HP,
   the torch / vine flags, the follow mode.

Do not grind; do not read other runs' movies. Stop when the run is ~40k frames past its start (the start is at ~219k), or
at a point you cannot get past after real attempts (say what blocks you). Report: run, frames, the route steps reached
with their frames, the end-save digests, each character's level + equipment at the end and the upgrades taken / skipped
(with why), named state, walkthrough corrections (what the guide got wrong for EN-old), tool gaps.
