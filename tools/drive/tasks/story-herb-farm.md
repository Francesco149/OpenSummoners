---
name: story-herb-farm
start: story-stella-crest
steps:
  - loaded into a room with control: world != 0 and control
  - left the start room: room != 0 and frame > 3000
forbid:
  - back at the title after loading: world == 0 and tcount == 5 and frame > 6000
---
**Frontier probe: Day 6's end, from the end of `story-stella-crest`** (start: that scenario's movie replayed —
f174304, **Arche (Lv5, cap 7, HP ~68/172, Iron Sword + Leather Armor) + Stella (Lv4)** in **290230** of the Barness
Hills Dungeon, just after Wind Crest 2, heading out the Stella way; ~1 Better Herb, 1 Herb, 1 Magic Candy, 1131 CP; last
save = the Weathervane Inn, slot 9, digest `0ef7e721d142`). Read `docs/findings/probes/story-stella-crest.md` and the
walkthrough's [RD story-stella-crest] note first (290240 / 290160 zones, the lever is V not ↑, respawned enemies in
290240). Heal first (F1 → Items, away from enemies = instant) and fight with care (`--hold` searches; add the room to
enemy-HP goals; `ehp` misses slots ≥ 6).

Next, per the walkthrough (Day 6, step 11): out of the dungeon with Stella (her route = shortcut) → heading home: the
boys, the **doctor** (medicine for Sana) → the **Barness herbalist** (first house) → back door → **Herb Farm Cave**
(Arche + Stella): crates + platform (do NOT break the crates first; Stella climbs, "Stay There!", Arche breaks and pushes)
→ [Better Herb] → vines → [Memory Photo 4] → crescent wall (block onto the blue button) → [Stronggirl Juice] → 3
scales-symbol torches → [Good Child Drop], [Revival Drink], 300 CP → **MoH 6** + the **fountain** (heal / SAVE) →
**BOSS: Baby Mage** (+2 Kobolds; `docs/game/bosses.md`) → [Topah Herb] → Sana's house cutscene → save.

**Upgrades (DRIVER.md "keep the party strong") — this leg's list:**
- **Equipment audit first:** Arche's Armor #2 and Accessory slots are EMPTY. Open the equip menu for both characters and
  fill every empty slot from the shared inventory (anything she/he can wear beats nothing: gloves, sashes, rings,
  ribbons). Report what each wears afterward.
- **MoH 6** (cap 9) in the Herb Farm Cave is on the route — take it. MoH 5 (cap 8, Barness Hills Dungeon, "next area
  left wooden door" after the diamond switch) was skipped by Arche's leg: take it only if the way out passes it cheaply.
- Stat items: **Good Child Drop → Stella**, **Stronggirl Juice → Stella** (check the item text), Power Drop → Arche.
- The Barness general store (270190, ↑ at lx ≈ 260000 at the right end of the inn street 270120) is on the way to the
  herbalist: buy Better Herbs (18 CP) for the cave + boss; the Shortsword (2800) only if the money allows.

1. `rd new <run> --from story-stella-crest` replays that movie into a live session. Confirm where you are: `rd look`,
   `rd text`, `rd status`.
2. Follow the walkthrough's 🔑 steps from there, as far as you can. Save at every goddess statue / fountain / inn save
   you pass and run `rd endsave <run>` after each save, so later probes can start there; note each digest.
3. Fights: read `docs/game/bosses.md` before a boss; use `--hold` searches (DRIVER.md); keep HP up. A game over is fine
   to see once (coverage) — then `rd undo` / `ck load` (note: `ck load` replays from boot, ~3 min — prefer `rd undo` or
   a fresh `ck save` near the fight).
4. **Run `rd check` and `rd verify` when you reach ~$6 spent** (the last two drivers ran out before verifying), then go on.
5. Name the state you need as you go (`rd ram R mark` → act → `rd ram R diff` → `rd ram R define`): the Baby Mage's HP,
   torch flags, Stella's follow / "Stay There!" state.

Do not grind; do not read other runs' movies. Stop when the run is ~40k frames past its start (the start is at ~174k), or
at a point you cannot get past after real attempts (say what blocks you). Report: run, frames, the route steps reached
with their frames, the end-save digests, each character's level + equipment at the end and the upgrades taken / skipped
(with why), named state, walkthrough corrections (what the guide got wrong for EN-old), tool gaps.
