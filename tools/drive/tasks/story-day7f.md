---
name: story-day7f
start: story-d7e
steps:
  - in control in the tunnel: world != 0 and control
  - left the start room: room != 330110 and room != 0
forbid:
  - back at the title: world == 0 and tcount == 5 and frame > 2000
---
**Frontier probe: Carudale Tunnels exit → home to Tonkiness (Day 7 end) → Day 8 start, from the end of run `story-d7e`**
(its movie replayed: f38431 in **330110**, control, just inside its west entry; Arche 163/197 HP, Sana 86/104 HP 35 MP;
1172 CP; `story` 1210; **[Taiyaki Sweet Sticks] in hand**). No save since the Chartreux inn (slot 13, digest
`5e899fce8699`) — a wipe means re-running from there, so play safe. **Arche Lv7: Iron Sword, Thick Leather, Woven Gloves
(Accessory empty); Sana Lv5: Iron Staff, Leather Armor, Chest Sash, Green Smock.** Items: Herb ×2, Better Herb ×7, Revival
Drink, Magic Candy ×2, Mentama Drop, Merrin Card, Kitty Ears, Underwear. Read
`/mnt/ephemeral/opensummoners/drive/agents/story-d7e/report.md` and the walkthrough's Day 7 step 7 ([RD story-d7e]) +
Day 8 steps 1–2 first. Map of the start room: `/mnt/ephemeral/opensummoners/drive/runs/story-d7e/view/map_330110_f38431.png`.

**Tools:** `rd zones R [--all]` on entering each room; `rd map R [--crop X0,Y0,X1,Y1] [--scale N]`; `rd ck R drop NAME`.
**Every `--score` search needs the current room in its `--avoid`** (`room != <id>`) or it maximises the score in the next
room. **Never mash `ent` through Yes/No prompts** (DRIVER.md). Do NOT allow `f2` in searches.

Next:
1. 🔑 330110: ↑ exit at (195200, 32000), up the zig-zag on the right (a Merkid Mage + a Lead Merkid on the way: kill the
   Mage first, it heals) → Loch Carudale 260230 → 260220 → 260215 → 260210 → 260200 (the crate) → 260190 → 260180 … →
   Tonkiness 210110 (the way story-d7b / earlier runs came, reversed; `rd zones` each room).
2. 🔑 Tonkiness: Sana home; talk to the parents; pajamas; save (home save) → `rd endsave`.
3. **Day 8** (if frames remain): class (Stella left without you) → exit school RIGHT (east road) → classmates, the wandering
   healer (heal / save) → Sana captured by Orgs. Stop there or at the Org Lair entrance.
Save at every inn / statue / home save and `rd endsave <run>` after each; note each digest.

Fights: `--hold` searches; keep an avoid on each member's HP; Heal Water between rooms; the Revival Drink is for
emergencies. **Run `rd check` and `rd verify` when you reach ~$6 spent (mandatory)**; verify again before `finish()` if the
budget allows. Do not grind; do not read other runs' movies. Stop when the run is ~40k frames past its start, or at a
point you cannot get past after real attempts (say what blocks you, with an `rd map` PNG path). Report: run, frames, route
steps with frames, end-save digests, each character's level + equipment at the end, upgrades taken / skipped (why), shop
lists + prices seen, named state, walkthrough corrections, tool gaps.
