---
name: story-day8e
start: save:b72717552038
steps:
  - loaded into a room with control: world != 0 and control
  - left the start room: room != 0 and frame > 3000
forbid:
  - back at the title after loading: world == 0 and tcount == 5 and frame > 6000
---
**Frontier probe: Korat Village → shops → the hot-spring problem → Mayor → Ellie → Kanon / the Fire Shrine, from the Warm
Breeze Inn save** (`rd endsave` digest `b72717552038`, run `story-d8d`, slot **19 "Korat Village"**, room 360170). **Loading:**
at the title `tcur` boots on 2 → press `U` before Continue; pick slot 19 by its number (check the page: `rd look`).
Party: **Arche Lv8** 206 HP (Org Slayer, Thick Leather, Leather Cape), **Sana Lv8** 136 HP / 132 MP (Iron Staff, Leather
Armor, Chest Sash, Green Smock), **Stella Lv8** 166 HP / 110 MP (Rondel, Woven Gloves, NO body armour); 1727 CP; ~2 Better
Herbs, Holy Water ×2, Magic Candy ×2, Revival Drink, Handmade Tart. Read
`/mnt/ephemeral/opensummoners/drive/agents/story-d8d/report.md` (it has Korat's room graph) and the walkthrough's Day 8
steps 4–6 ([RD story-d8d]) first.

**Tools:** `rd zones R [--all]`; `rd map R` (misses jump-through platforms); `rd look` to read menus; room exits can be
read from memory (area table `s32(world+0x103c)`, 336-byte records, zone table +0x1c). Drop checkpoints sparingly.
**Every `--score` search needs the current room in its `--avoid`.** Never mash `ent` through Yes/No prompts or shop
lists (Enter on a list BUYS); do NOT allow `f2` in searches.

Next:
1. Korat shops (weapon store, item store — both pages; food / souvenir store): record every list + price. Buy **body
   armour for Stella**, **Venom-Away** ×3+, Better Herbs, a [Recall Lamp] if affordable; clear upgrades for Sana. 💎
   cheap loot on the way (walkthrough step 4: pots, drawers, closets).
2. 🔑 Inn ground-floor door (83200, 54400) → 360175 → hot-spring cutscene (springs off) → **Mayor** → **Ellie** (Inn 1F
   fireplace) → Kanon went to the Fire Shrine (far east). Save at the inn before leaving (`rd endsave`).
3. **Fire Shrine** (if frames remain): right, climb steps → 💎 [Mahogany Staff] (for Sana); drop, go left → **BOSS: Great
   Scorpion** (save first). Stop at the boss, or after it.
Save at every save point and `rd endsave <run>` after each; note each digest.

Fights: `--hold` searches; keep an avoid on each member's HP; Venom-Away against the Mothbees; Heal Water between rooms;
the Revival Drink is for emergencies. **Run `rd check` and `rd verify` when you reach ~$6 spent (mandatory)**; verify
again before `finish()` if the budget allows. Do not grind; do not read other runs' movies. Stop when the run is ~40k
frames past its start, or at a point you cannot get past after real attempts (say what blocks you, with an `rd map` PNG
path). Report: run, frames, route steps with frames, end-save digests, each character's level + equipment at the end,
upgrades taken / skipped (why), shop lists + prices seen, named state, walkthrough corrections, tool gaps.
