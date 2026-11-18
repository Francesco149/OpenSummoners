---
name: story-day8f
start: story-d8e
steps:
  - in control in the shrine: world != 0 and control
  - left the start room: room != 380135 and room != 0
forbid:
  - back at the title: world == 0 and tcount == 5 and frame > 2000
---
**Frontier probe: the Fire Shrine → BOSS Great Scorpion → Kanon → the pamphlet password, from the end of run
`story-d8e`** (its movie replayed: f39413 in **380135** (the Mahogany Staff chest room), control, Sana leading). Party:
**Arche Lv9** 132/209 HP (Org Slayer, Thick Leather, Leather Cape, Kitty Ears), **Sana Lv8 53/139 HP** 118/134 MP
(Mahogany Staff, Leather Armor, Chest Sash, Green Smock) — **heal her first**, **Stella Lv8** 150/166 HP (Rondel, Woven
Gloves, no body armour); 1232 CP; Better Herb ×5, Venom-Away S ×3, Hot Springs Bun ×3 (200–250 HP), Holy Water ×2, Magic
Candy ×2, Mind Chocolate, Revival Drink, **Recall Lamp**. Last save: Warm Breeze Inn, slot 20, digest `6700850a55ab` (before
the shrine). Read `/mnt/ephemeral/opensummoners/drive/agents/story-d8e/report.md` and the walkthrough's Day 8 step 6
([RD story-d8e]) first.

**Tools:** `rd zones R [--all]`; `rd map R` (misses jump-through platforms); `rd look` to read menus (`control` stays
true with F1 open); room exits from memory (area table `s32(world+0x103c)`, 336-byte records, zone table +0x1c). Bound
`lx` in `--avoid` so `--score` searches don't overshoot doors. **Every `--score` search needs the current room in its
`--avoid`.** Never mash `ent` through Yes/No prompts or shop lists; do NOT allow `f2` in searches.

Next:
1. Heal (Sana's Heal Water / a Hot Springs Bun). 🔑 Find the boss route: walkthrough says "drop, go left" after the
   staff; candidates: down from 380135 / 380130, or 380110 zone 3 (412800, 67200) → 380120. Map the shrine's room graph
   from memory first.
2. 🔑 **BOSS: Great Scorpion** (full HP/MP first; Venom-Away if poisoned) → Kanon → back to the inn → the pamphlet →
   **password** ([Password Note]). Save at the inn (`rd endsave`).
3. If frames remain: the walkthrough's step 7 (Korat Caves, the sealed door with the password). Stop at the next boss.
Save at every save point and `rd endsave <run>` after each; note each digest.

Fights: `--hold` searches; keep an avoid on each member's HP; Heal Water between rooms; the Revival Drink is for
emergencies; the Recall Lamp returns to town. **Run `rd check` and `rd verify` when you reach ~$6 spent (mandatory)**;
verify again before `finish()` if the budget allows. Do not grind; do not read other runs' movies. Stop when the run is
~40k frames past its start, or at a point you cannot get past after real attempts (say what blocks you, with an `rd map`
PNG path). Report: run, frames, route steps with frames, end-save digests, each character's level + equipment at the end,
upgrades taken / skipped (why), shop lists + prices seen, named state, walkthrough corrections, tool gaps.
