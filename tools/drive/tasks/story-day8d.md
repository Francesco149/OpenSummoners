---
name: story-day8d
start: story-d8c
steps:
  - in control in the valley: world != 0 and control
  - left the start room: room != 345240 and room != 0
forbid:
  - back at the title: world == 0 and tcount == 5 and frame > 2000
---
**Frontier probe: Cranbal Valley (after MoH 9) → Korat Village → the hot-spring problem → Kanon at the Fire Shrine,
from the end of run `story-d8c`** (its movie replayed: f114535 in **345240** after MoH 9, control; Arche Lv8 206 HP
(Org Slayer, Thick Leather, Woven Gloves), Sana Lv7 131 HP / 125 MP (Iron Staff, Leather Armor, Chest Sash, Green
Smock; Turbid Squall new), Stella Lv8 166 HP / 110 MP (Rondel, NO armour / accessory); 1445 CP; ~6–8 Better Herbs, a
Revival Drink, 2 Magic Candy). Last save: the healer, slot 18, digest `43128e19cdfa` (before MoH 9). Read
`/mnt/ephemeral/opensummoners/drive/agents/story-d8c/report.md` and the walkthrough's Day 8 steps 3–6 ([RD story-d8c])
first.

**Tools:** `rd zones R [--all]`; `rd map R` (does NOT show jump-through ledges — try jumping up through ceilings that
look solid); room exits/doors can be read from memory (area table `s32(world+0x103c)`, 336-byte room records, zone table
at +0x1c — see the d8c report) when a route is unclear. A position `--goal` often fires mid-jump: add `--hold`. Drop
checkpoints sparingly. **Every `--score` search needs the current room in its `--avoid`.** Never mash `ent` through
Yes/No prompts or shop lists (Enter on a list BUYS); do NOT allow `f2` in searches.

Next:
1. 🔑 345240 → 345230 (top-right walk-in) → 345210 zone 2 (150400, 89600) → 345120 → 345220 → 345130 → 340190 → 340200
   → 360110 (Korat?). Optional if cheap on the way: [Leather Cape] (bottom-left cave), diamond lever, [Holy Water],
   [Good Child Drop].
2. Korat: weapon / item store lists + prices (both pages); buy Stella armour + clear upgrades; the Inn (save → `rd
   endsave`). 🔑 Inn ground-floor door → hot spring cutscene → Mayor → Ellie (Inn 1F fireplace) → Kanon went to the Fire
   Shrine. Stop there, or at the Fire Shrine entrance if frames remain.
Save at every save point and `rd endsave <run>` after each; note each digest.

Fights: `--hold` searches; keep an avoid on each member's HP; Heal Water between rooms; the Revival Drink is for
emergencies. **Run `rd check` and `rd verify` when you reach ~$6 spent (mandatory)**; verify again before `finish()` if the
budget allows. Do not grind; do not read other runs' movies. Stop when the run is ~40k frames past its start, or at a
point you cannot get past after real attempts (say what blocks you, with an `rd map` PNG path). Report: run, frames, route
steps with frames, end-save digests, each character's level + equipment at the end, upgrades taken / skipped (why), shop
lists + prices seen, named state, walkthrough corrections, tool gaps.
