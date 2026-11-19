---
name: story-day10a
start: story-d9f
steps:
  - in control on Day 10: world != 0 and control
  - left the manor: room != 310120 and room != 310130 and room != 310140 and room != 0
forbid:
  - back at the title: world == 0 and tcount == 5 and frame > 2000
---
**Frontier probe: Day 10 — from Mayberk Manor to the Weathervane Tower (walkthrough Day 10 steps 1–2.4), from the end of
run `story-d9f`** (its movie replayed: f337196 in **310120**, the Day 10 morning scene playing, `script` 1; slot 23 save
`0f2d49481120` made just before). Party **Lv12**: **Arche** 255 HP (Org Slayer, Thick Leather, Leather Cape, Kitty
Ears; Leaping Slash learned, unassigned), **Sana** 173 HP / 208 MP (Mahogany Staff, Leather Armor, Chest Sash, Green
Smock, Anti-Poison Ring; water magic), **Stella** 218 HP / 156 MP (Lady's Dagger, Scale Armor, Woven Gloves, Salamander
Scale; Phoenix slot 3); **10802 CP**; Ghost Marshmallow ×3, Better Herb, Revival Drink, Venom-Away S ×3, Recall Lamp,
Doll Hand (unknown), **[Thieves' Key]**. Read `/mnt/ephemeral/opensummoners/drive/agents/story-d9f/report.md` and the
walkthrough's **Day 10** first.

**Tools:** `rd zones R --all` (doors, objects + "zone hold"); object state `s32(s32(world+0x11e0+4*slot)+0xc)`; `rd map
R --crop`; `rd ck R save/load/drop` (loading an OLDER checkpoint drops every later one). Keep `{…}` inputs short.
Every `--score` search needs the current room in its `--avoid`; bound `lx`. **Never `rd tap ent` through dialogue that
may end in a Yes/No or save prompt** — tap a few at a time and `rd look`; do NOT allow `f2` in searches.

Next:
1. Let the morning scene play out (watch `script` / text). 💎 Chartreux shops on the way (10802 CP): first **armour /
   weapon upgrades** (Arche still has Thick Leather; Sana Leather Armor) — note the stock + prices; the night-only
   [Ghost Blaster] 12,500 is no longer needed. Assign Arche's Leaping Slash if the menu allows.
2. 🔑 Chartreux → right past the White Rose Inn → the Weathervane Tower road → the Thieves' Key opens the tower. Save
   at the White Rose Inn on the way (`rd endsave`).
3. 🔑 **Weathervane Tower** (Young Harpy, Harpy, Rattlesnake, Kobold Ace; harpies grab and drop you a floor; harpy song
   confuses): 2.1 ⚠ the 1st-floor side chests are spike traps; centre: climb the ledges from the left, then the right
   ledge → stairs. 2.2 **2 unlit torches: do NOT light them — Sana's water puts ALL out** → door. 2.3 💎 far right stairs
   down: moving blocks over spikes → [Rune Staff] (Sana). 2.4 ride the platform up, push the block → lever; far right
   [Revival Drink]; zigzag → [Water of Life]. Stop after 2.4 (or at ~40k frames).
Save at every save point and `rd endsave <run>` after each; note each digest.

Fights: `--hold` searches; keep an avoid on each member's HP. Do not grind; do not read other runs' movies. **Run `rd
check` and `rd verify` when you reach ~$6 spent (mandatory)**; verify again before `finish()` if the budget allows. Stop
when the run is ~40k frames past its start, or at a point you cannot get past after real attempts (say what blocks you,
with an `rd map` PNG path). Report: run, frames, route steps with frames, end-save digests, each character's level +
equipment at the end, upgrades taken / skipped (why), shop lists + prices seen, named state, walkthrough corrections,
tool gaps.
