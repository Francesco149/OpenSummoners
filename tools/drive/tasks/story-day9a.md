---
name: story-day9a
start: story-d8i
steps:
  - in control on Day 9: world != 0 and control
  - left the school: room != 230140 and room != 0
forbid:
  - back at the title: world == 0 and tcount == 5 and frame > 2000
---
**Frontier probe: Day 9 — school (Stella's invitation) → Sana in the barn → the moms' permission → Chartreux shops →
Mayberk Manor → Rowen → dinner → night: the moaning → Chartreux Graveyard, from the end of run `story-d8i`** (its movie
replayed: f172512 in **230140**, the school at the start of Day 9; the Day 8 save `328360c66c95` was made at f172268).
Party: **Arche Lv10** (Org Slayer, Thick Leather, Leather Cape, Kitty Ears), **Sana Lv10** (Mahogany Staff, Leather Armor,
Chest Sash, Green Smock, Anti-Poison Ring), **Stella Lv10** (Lady's Dagger, Woven Gloves, Salamander Scale; no body
armour); 6146 CP (cap 40,000); Better Herb ×3, Venom-Away S, Revival Drink, Sweet Sticks ×2. Read
`/mnt/ephemeral/opensummoners/drive/agents/story-d8i/report.md` and the walkthrough's **Day 9** first.

**Tools:** `rd zones R` (doors + "zone hold": which foe holds an exit next to which member); `rd map R --crop` (world
units); `rd ck R drop NAME` early (the savestate pool is small). Every `--score` search needs the current room in its
`--avoid`; bound `lx`. Never mash `ent` through Yes/No prompts or save prompts (`rd tap --until control` walks through a
save prompt — stop at `script`/text changes instead); do NOT allow `f2` in searches.

Next:
1. 🔑 School: Stella's invitation; Sana in the barn → talk; Tonkiness: Mom + Sana's mom permission (walkthrough Day 9
   step 1). 💎 Optional, only if cheap: the Barness road weight cave → [Calming Cocoa].
2. Chartreux shops (step 3): note the new stock + prices; buy **body armour for Stella** and MP items (Magic Candy) if
   affordable — the catacombs are long. Save at a save point on the way (`rd endsave`).
3. 🔑 **Mayberk Manor** (step 4): Stella's room → talk to everyone → evening → **Rowen** → dinner → night: moans. 💎
   Weapon store at NIGHT → [Ghost Blaster] 12,500 if affordable. Kitchen back door → **Chartreux Graveyard** (Vile Bats;
   Sana appears) → push the first pushable gravestone fully left → the Catacombs entrance. Save before entering if a
   save point allows. Stop at the Catacombs' first room.
Save at every save point and `rd endsave <run>` after each; note each digest.

Fights: `--hold` searches; keep an avoid on each member's HP. Do not grind; do not read other runs' movies. **Run `rd
check` and `rd verify` when you reach ~$6 spent (mandatory)**; verify again before `finish()` if the budget allows. Stop
when the run is ~40k frames past its start, or at a point you cannot get past after real attempts (say what blocks you,
with an `rd map` PNG path). Report: run, frames, route steps with frames, end-save digests, each character's level +
equipment at the end, upgrades taken / skipped (why), shop lists + prices seen, named state, walkthrough corrections,
tool gaps.
