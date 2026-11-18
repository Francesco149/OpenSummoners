---
name: story-day8c
start: story-d8b
steps:
  - in control after the lair: world != 0 and control
  - left the start room: room != 350200 and room != 0
forbid:
  - back at the title: world == 0 and tcount == 5 and frame > 2000
---
**Frontier probe: after the Org Lair → Cranbal Valley (MoH 9) → Korat Village, from the end of run `story-d8b`** (its
movie replayed: f81525 in **350200** after MoH 8, control, **3-member party**: Arche Lv7 197 HP (Org Slayer, Thick
Leather, Woven Gloves, no accessory), Sana Lv6 112 HP / 117 MP (gear unchecked — her old gear may be in the bag), Stella
Lv7 157 HP / 88 MP (just learned Phoenix); 2301 CP). Last save: the lair fountain, slot 17, digest `f83f6b01986d`.
Read `/mnt/ephemeral/opensummoners/drive/agents/story-d8b/report.md` and the walkthrough's Day 8 steps 3–6 first.

**First:** F1 menu — check/equip Sana's gear and Stella's (and her skills / Magic Usage), cheaply (step choices by hand).

**Tools:** `rd zones R [--all]`; `rd map R [--crop …] [--scale N]`; drop checkpoints sparingly (pool ~13–21; each drop
rebuilds the session). Full jump `C*30` for tall ledges. Exits often fire only on ↑ while standing still with no foe
near. **Every `--score` search needs the current room in its `--avoid`.** Never mash `ent` through Yes/No prompts; do
NOT allow `f2` in searches.

Next:
1. 🔑 Right → **Cranbal Valley** (Poison Mothbee, Sabercat, Vicious Cocorat): 💎 bottom-left cave → [Leather Cape];
   tunnel down → **MoH 9**. Exit right; Sana fetches the boat; up ledges → cave → exit left (diamond lever, [Holy Water]);
   back in, top right → moving platform → **Korat Village** (red steaming hot springs).
2. Korat: note the weapon / item store lists + prices; buy clear upgrades (Sana / Stella first); the Inn (save →
   `rd endsave`). 🔑 Inn ground floor door → hot spring cutscene → Mayor → Ellie → Kanon went to the Fire Shrine. Stop
   there, or at the Fire Shrine entrance if frames remain.
Save at every save point and `rd endsave <run>` after each; note each digest.

Fights: `--hold` searches; keep an avoid on each member's HP; Heal Water between rooms; the Revival Drink is for
emergencies. **Run `rd check` and `rd verify` when you reach ~$6 spent (mandatory)**; verify again before `finish()` if the
budget allows. Do not grind; do not read other runs' movies. Stop when the run is ~40k frames past its start, or at a
point you cannot get past after real attempts (say what blocks you, with an `rd map` PNG path). Report: run, frames, route
steps with frames, end-save digests, each character's level + equipment at the end, upgrades taken / skipped (why), shop
lists + prices seen, named state, walkthrough corrections, tool gaps.
