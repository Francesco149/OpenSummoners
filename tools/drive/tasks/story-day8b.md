---
name: story-day8b
start: story-d8a
steps:
  - in control in the lair: world != 0 and control
  - left the start room: room != 350150 and room != 0
forbid:
  - back at the title: world == 0 and tcount == 5 and frame > 2000
---
**Frontier probe: the Org Lair, from the gargoyle room → fountain → Boss Org → MoH 8 → Sana rescued / Stella joins,
from the end of run `story-d8a`** (its movie replayed: f40663 in **350150**, control; Arche alone, 197/197 HP, 31/31 MP,
Lv7, **Org Slayer** (Attack 57), Thick Leather, Woven Gloves; 2085 CP; Herbs, Better Herbs, Revival Drink, Handmade
Tarts, Magic Candy). Last save: the healer, slot 16, digest `0ba881e290ac`. Read
`/mnt/ephemeral/opensummoners/drive/agents/story-d8a/report.md` and the walkthrough's Day 8 step 2.3–2.4 + 3 ([RD
story-d8a]) first; the start-room map: `/mnt/ephemeral/opensummoners/drive/agents/story-d8a/map_350150_f40663.png`.

**Tools:** `rd zones R [--all]`; `rd map R [--crop …] [--scale N]`; drop checkpoints sparingly (each drop replays the
movie: ~5 min). Add the full jump `C*30` to climbing searches. Exits often fire only on ↑ while standing still with no
foe near. **Every `--score` search needs the current room in its `--avoid`.** Never mash `ent` through Yes/No prompts.
Do NOT allow `f2` in searches.

Next:
1. 🔑 350150: push the **gargoyle onto the button** → wooden door; 💎 chest (190400, 102400) (likely [Power Drop]) →
   cave with many Orgs → top-left wooden door: the platform + statue puzzle (walkthrough 2.3, step by step).
2. 🔑 Back: the heart wooden platform → tunnel; 💎 left [Better Herb]; right past archers → door → stairs → **fountain**
   (save: `rd endsave`).
3. 🔑 Left → fancy doors → **BOSS: Boss Org** (+ Thuggish Org, Org Lackey). Save first, full HP/MP; use the Org Slayer.
   Then stairs down → **MoH 8**; Sana rescued; note when Stella joins. Stop there, or at Cranbal Valley if frames remain.
Save at every save point and `rd endsave <run>` after each; note each digest.

Fights: `--hold` searches; keep an avoid on Arche's HP; the Revival Drink is for emergencies. **Run `rd check` and `rd
verify` when you reach ~$6 spent (mandatory)**; verify again before `finish()` if the budget allows. Do not grind; do not
read other runs' movies. Stop when the run is ~40k frames past its start, or at a point you cannot get past after real
attempts (say what blocks you, with an `rd map` PNG path). Report: run, frames, route steps with frames, end-save digests,
each character's level + equipment at the end, upgrades taken / skipped (why), shop lists + prices seen, named state,
walkthrough corrections, tool gaps.
