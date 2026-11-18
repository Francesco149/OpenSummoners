---
name: story-day9c
start: story-d9b
steps:
  - in control in the Catacombs: world != 0 and control
  - left the first Catacombs room: room != 400110 and room != 0
forbid:
  - back at the title: world == 0 and tcount == 5 and frame > 2000
---
**Frontier probe: Chartreux Catacombs, walkthrough Day 9 steps 4.1–4.3 (Crypt Keys, [Scale Armor], the 3 floor buttons
→ MoH 12), from the end of run `story-d9b`** (its movie replayed: f234522 in **400110**, the Catacombs' first room (lx
30400, ly 29600), free roam, sword drawn, party 7, Arche leading, 2 Ghosts (51081) ahead). Party Lv10: **Arche** 137/216
(Org Slayer, Thick Leather, Leather Cape, Kitty Ears), **Sana** 104/149, 156 MP (Mahogany Staff, Leather Armor, Chest
Sash, Green Smock, Anti-Poison Ring), **Stella** 130/189, 120 MP (Lady's Dagger, Woven Gloves, Salamander Scale; **no
body armour**); 5261 CP; Better Herb ×9, Magic Candy ×4, Recall Lamp, Revival Drink, Venom-Away S ×2. Last save: White
Rose Inn, slot 22, `a2a16253c9bb` (no save since). Read `/mnt/ephemeral/opensummoners/drive/agents/story-d9b/report.md`
and the walkthrough's **Day 9** step 4 ([RD story-d9b]) first.

**Tools:** `rd zones R` (doors + "zone hold"); `rd map R --crop` (world units); `rd ck R drop NAME` early (the bunker's
savestate pool is small; a lost pool = a ~9-minute replay). Prefer fixed key counts over per-frame `{…}` inputs (those
run very slowly). Every `--score` search needs the current room in its `--avoid`; bound `lx`. Never mash `ent` through
Yes/No or save prompts; do NOT allow `f2` in searches. Pushing: sword sheathed, from the side you push away from.

Next:
1. 🔑 4.1: the 2 Ghosts → chest [Crypt Key] (400110 chest 70230 at (114200, 22400)?) → unlock the right door, stairs
   down; left [Recall Lamp].
2. 🔑 4.2: right: Stella burns the vines (Fire), door; drop the pit hugging LEFT → ledge → [Crypt Key]; left door;
   spikes; use the key; up: [Mystery Juice], **[Scale Armor] → equip on Stella**.
3. 🔑 4.3: down, lever; bottom-left burn vines, pot 1200 CP (pots open with the sword drawn), tunnel → **3 floor
   buttons**: park each member on one (F1 Follow Mode "Stay There!") → the right wall opens → **MoH 12** + [Crypt Key].
   Stop after MoH 12 (or at ~40k frames).
Save at every save point and `rd endsave <run>` after each; note each digest.

Fights: `--hold` searches; keep an avoid on each member's HP; ghosts may resist the sword — note what hurts them. Do
not grind; do not read other runs' movies. **Run `rd check` and `rd verify` when you reach ~$6 spent (mandatory)**;
verify again before `finish()` if the budget allows. Stop when the run is ~40k frames past its start, or at a point you
cannot get past after real attempts (say what blocks you, with an `rd map` PNG path). Report: run, frames, route steps
with frames, end-save digests, each character's level + equipment at the end, upgrades taken / skipped (why), shop
lists + prices seen, named state, walkthrough corrections, tool gaps.
