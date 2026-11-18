---
name: story-day8i
start: story-d8h
steps:
  - in control in the caves: world != 0 and control
  - past the block room: room != 370180 and room != 0 and room != 370175
forbid:
  - back at the title: world == 0 and tcount == 5 and frame > 2000
---
**Frontier probe: Korat Caves 370180 block / lever / vent puzzle → 370190 → 370200 → 370210 BOSS New Age Myconid →
MoH 11 → hot spring → save, from the end of run `story-d8h`** (its movie replayed: f131926 in **370180** at the entry
(0, 112800), Arche leading, sword drawn, no foe near; the clover lever is ON). Party: **Arche Lv10** 147/216 HP 39 MP
(Org Slayer, Thick Leather, Leather Cape, Kitty Ears), **Sana Lv10** 149/149 HP **1/161 MP** (Mahogany Staff, Leather
Armor, Chest Sash, Green Smock, Anti-Poison Ring), **Stella Lv10** 162/189 HP 40/132 MP (Lady's Dagger, Woven Gloves,
Salamander Scale; Phoenix = spell slot 3); 2241 CP; Better Herb ×2, Hot Springs Bun ×3, Handmade Tart, Sweet Sticks ×5,
Revival Drink, Recall Lamp, Venom-Away S. **No MP items.** No save since the Korat inn (slot 21, `4ce2d13efdf6`). Read
`/mnt/ephemeral/opensummoners/drive/agents/story-d8h/report.md` and the walkthrough's Day 8 step 7 ([RD story-d8g],
[RD story-d8h]) first.

**Tools:** `rd zones R` now also prints **"zone hold"**: which foe holds the exit next to which member (leader within
56000, every active member within 48000, 20000 in y) — use it instead of guessing. `rd map R`. Every `--score` search
needs the current room in its `--avoid`; bound `lx`. Never mash `ent` through Yes/No prompts; do NOT allow `f2` in
searches. Keep named checkpoints few (bunker's savestate pool is small); `rd stop` is not needed between commands.

Next:
1. 🔑 370180 (walkthrough 7.4): up into the top region; push both blocks (192000 / 216000, y 22400) off the ledge, right
   one first; push the stack right over the floor vent (236800, 20800); flip lever 511 (257600, 33600) → vent 512 by
   the exit off → zone 2 (35200, 38400; press ↑) → 370190. Chests 501 / 502 if cheap.
2. → 370200 → 370210: cutscene → **BOSS: New Age Myconid** (+ Myconid, Toxic Myconid; spores = sleep / poison).
   **MP is short** (Sana 1, no MP items): check whether MP regenerates while idle (watch `m0..m2` over a few hundred
   standing frames) before the boss; Arche's sword + Stella's Lady's Dagger melee may have to carry it. Full HP first.
3. 🔑 Wooden door right → **MoH 11** + [Better Herb]×3 → exit (Recall Lamp) → hot spring scene → save (`rd endsave`).
   Stop there.
Save at every save point and `rd endsave <run>` after each; note each digest.

Fights: `--hold` searches; keep an avoid on each member's HP. Do not grind; do not read other runs' movies. **Run `rd
check` and `rd verify` when you reach ~$6 spent (mandatory)**; verify again before `finish()` if the budget allows. Stop
when the run is ~40k frames past its start, or at a point you cannot get past after real attempts (say what blocks you,
with an `rd map` PNG path). Report: run, frames, route steps with frames, end-save digests, each character's level +
equipment at the end, upgrades taken / skipped (why), shop lists + prices seen, named state, walkthrough corrections,
tool gaps.
