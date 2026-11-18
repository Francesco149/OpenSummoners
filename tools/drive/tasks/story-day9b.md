---
name: story-day9b
start: story-d9a
steps:
  - in control in the manor: world != 0 and control
  - left the hall: room != 310130 and room != 0
forbid:
  - back at the title: world == 0 and tcount == 5 and frame > 2000
---
**Frontier probe: Mayberk Manor → Stella's room → talk to everyone → evening → Rowen → dinner → night: the moaning →
(Ghost Blaster check) → kitchen back door → Chartreux Graveyard → the gravestone → Catacombs entrance, from the end of run
`story-d9a`** (its movie replayed: f209786 in **310130**, the manor hall's upper floor far left (lx 12808, ly 25700),
control, 3-member party, `story` 1420). Party Lv10: **Arche** (Org Slayer, Thick Leather, Leather Cape, Kitty Ears),
**Sana** (Mahogany Staff, Leather Armor, Chest Sash, Green Smock, Anti-Poison Ring), **Stella** (Lady's Dagger, Woven
Gloves, Salamander Scale; **no body armour**); 5139 CP; Better Herb ×9, Magic Candy ×4, Recall Lamp, Revival Drink,
Venom-Away S ×2. Last save: White Rose Inn, slot 22, `a2a16253c9bb`. Read
`/mnt/ephemeral/opensummoners/drive/agents/story-d9a/report.md` and the walkthrough's **Day 9** ([RD story-d9a]) first.

**Tools:** `rd zones R` (doors + "zone hold"); `rd map R --crop` (world units); `rd ck R drop NAME` early. Every
`--score` search needs the current room in its `--avoid`; bound `lx`. Never mash `ent` through Yes/No or save prompts
(`rd tap --until control` walks through a save prompt); do NOT allow `f2` in searches. NPC talk: some answer only V,
some only from one side.

Next:
1. 🔑 Manor (walkthrough Day 9 step 4): Stella's room (310140's top floor? its ladder did not climb with `U*40` — try
   other approaches / rooms; `rd zones` each room) → talk to everyone → evening → **Rowen** → dinner → night: moans.
2. 💎 At night the weapon store (300220) sells [Ghost Blaster] 12,500 — note whether it appears (5139 CP: likely not
   affordable; don't grind). Consider **Bronze Armor 2600 for Stella** only if the Catacombs' free [Scale Armor] looks
   far; otherwise keep the money.
3. 🔑 Kitchen back door → **Chartreux Graveyard** (Vile Bats; Sana appears) → push the first pushable gravestone fully
   left → secret entrance → the Catacombs' first room. Save first if a save point allows. Stop in the Catacombs' first
   room.
Save at every save point and `rd endsave <run>` after each; note each digest.

Fights: `--hold` searches; keep an avoid on each member's HP. Do not grind; do not read other runs' movies. **Run `rd
check` and `rd verify` when you reach ~$6 spent (mandatory)**; verify again before `finish()` if the budget allows. Stop
when the run is ~40k frames past its start, or at a point you cannot get past after real attempts (say what blocks you,
with an `rd map` PNG path). Report: run, frames, route steps with frames, end-save digests, each character's level +
equipment at the end, upgrades taken / skipped (why), shop lists + prices seen, named state, walkthrough corrections,
tool gaps.
