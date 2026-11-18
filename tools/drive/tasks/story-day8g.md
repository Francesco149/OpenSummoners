---
name: story-day8g
start: story-d8f
steps:
  - in control in the caves: world != 0 and control
  - left the start room: room != 370120 and room != 0
forbid:
  - back at the title: world == 0 and tcount == 5 and frame > 2000
---
**Frontier probe: the Korat Caves → BOSS New Age Myconid → MoH 11 → hot spring → save, from the end of run `story-d8f`**
(its movie replayed: f71259 in **370120**, control, all alive, steam lever already flipped, MoH 10 taken). Party: **Arche
Lv9** 209 HP (Org Slayer, Thick Leather, Leather Cape, Kitty Ears), **Sana Lv9** 132/145 HP (Mahogany Staff, Leather
Armor, Chest Sash, Green Smock; Heal Water II), **Stella Lv9** 107/182 HP (Rondel, Woven Gloves, no body armour;
Phoenix, Disintegrate); 1565 CP; Magic Candy ×3, Recall Lamp, Revival Drink, Venom-Away S, heals; an unidentified
[Salamander Scale]. Last save: the Korat inn, slot 21, digest `4ce2d13efdf6` (before the caves). Read
`/mnt/ephemeral/opensummoners/drive/agents/story-d8f/report.md` and the walkthrough's Day 8 step 7 ([RD story-d8f]) first.

**Tools:** `rd zones R [--all]`; `rd map R` (misses slopes / ledges / jump-through platforms); room exits from memory
(area table `s32(world+0x103c)`, 336-byte records, zone table +0x1c) — map the caves' graph first. Bound `lx` in
`--avoid` for `--score` searches (`toward()` overshoots after dashes). **Every `--score` search needs the current room in
its `--avoid`.** Never mash `ent` through Yes/No prompts; do NOT allow `f2` in searches.

Next:
1. Check the [Salamander Scale] (F1 → Items / equip if useful). 🔑 The caves' puzzles (walkthrough 7.2–7.4): push block
   over the vent → [Good Child Drop], [Orange Smock]; bottom area: [Mystery Juice], **[Anti-Poison Ring]** (equip it — the
   boss is a Myconid); the moving stone block + start/stop lever (Arche rides, another member works the lever) → the
   **clover switch**; then the lower path, [Guardian Drop], 3000 CP, the block stack over the vent → lever → upper-left
   vent off → tunnel.
2. 🔑 **BOSS: New Age Myconid** (+ Myconid, Toxic Myconid; spores = sleep / poison): full HP/MP first; Stella's Phoenix.
3. 🔑 Wooden door right → **MoH 11** + [Better Herb]×3 → exit (the Recall Lamp saves time) → hot spring scene → save
   (`rd endsave`). Stop there.
Save at every save point and `rd endsave <run>` after each; note each digest.

Fights: `--hold` searches; keep an avoid on each member's HP; Heal Water between rooms; the Revival Drink is for
emergencies. **Run `rd check` and `rd verify` when you reach ~$6 spent (mandatory)**; verify again before `finish()` if the
budget allows. Do not grind; do not read other runs' movies. Stop when the run is ~40k frames past its start, or at a
point you cannot get past after real attempts (say what blocks you, with an `rd map` PNG path). Report: run, frames, route
steps with frames, end-save digests, each character's level + equipment at the end, upgrades taken / skipped (why), shop
lists + prices seen, named state, walkthrough corrections, tool gaps.
