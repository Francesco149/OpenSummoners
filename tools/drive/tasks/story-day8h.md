---
name: story-day8h
start: story-d8g
steps:
  - in control in the caves: world != 0 and control
  - left the clover room: room != 370157 and room != 0
forbid:
  - back at the title: world == 0 and tcount == 5 and frame > 2000
---
**Frontier probe: Korat Caves clover puzzle → BOSS New Age Myconid → MoH 11 → hot spring → save, from the end of run
`story-d8g`** (its movie replayed: f108586 in **370157**, control, all alive, no foes in the room). Party: **Arche Lv10**
112/216 HP (Org Slayer, Thick Leather, Leather Cape, Kitty Ears), **Sana Lv9** 74/145 HP 47/150 MP (Mahogany Staff,
Leather Armor, Chest Sash, Green Smock, Anti-Poison Ring; Heal Water II), **Stella Lv9** 182 HP **2/118 MP** (Lady's
Dagger, Woven Gloves, Salamander Scale; Phoenix); 2044 CP; Magic Candy ×0, Mind Chocolate, Better Herb ×5, Hot Springs
Bun ×3, Revival Drink, Recall Lamp, Venom-Away S. No save since the Korat inn (slot 21, `4ce2d13efdf6`). Read
`/mnt/ephemeral/opensummoners/drive/agents/story-d8g/report.md` (cave graph, zone-hold rule, leader-switch trick) and
the walkthrough's Day 8 step 7 ([RD story-d8g]) first.

**Tools:** `rd zones R [--all]` (kind "walk in" may need ↑ while idle); `rd map R`; room exits from memory (area table
`s32(world+0x103c)`). **Zone hold counts every party member within 48000 of a foe** — a stranded follower blocks exits:
switch leader to it. Bound `lx` in `--avoid` for `--score` searches; **every `--score` search needs the current room in
its `--avoid`.** Never mash `ent` through Yes/No prompts; do NOT allow `f2` in searches.

Next:
1. Restore first: Sana's Heal Water to full; Mind Chocolate / Hot Springs Bun for Stella's MP (she carries the boss).
2. 🔑 370157 clover puzzle (walkthrough 7.3): start/stop lever (53200, 34800) + moving block (62800, 41600) — one member
   rides the block under the ceiling vents, another works the lever; flip the **clover lever (136000, 33600)**.
3. 🔑 Back to 370150 → zone 3 → the boss line 370160 → … → 370210 (walkthrough 7.4: moving platform, [Guardian Drop],
   3000 CP, block stack over the vent → lever → upper-left vent off → tunnel → **BOSS: New Age Myconid** (+ Myconid,
   Toxic Myconid; spores = sleep / poison)). Full HP/MP before the boss.
4. 🔑 Wooden door right → **MoH 11** + [Better Herb]×3 → exit (Recall Lamp) → hot spring scene → save (`rd endsave`).
   Stop there.
Save at every save point and `rd endsave <run>` after each; note each digest.

Fights: `--hold` searches; keep an avoid on each member's HP; Dragon Pups fly — Stella's Phoenix reaches them, the
sword rarely does. Do not grind; do not read other runs' movies. **Run `rd check` and `rd verify` when you reach ~$6
spent (mandatory)**; verify again before `finish()` if the budget allows. Stop when the run is ~40k frames past its
start, or at a point you cannot get past after real attempts (say what blocks you, with an `rd map` PNG path). Report:
run, frames, route steps with frames, end-save digests, each character's level + equipment at the end, upgrades taken /
skipped (why), shop lists + prices seen, named state, walkthrough corrections, tool gaps.
