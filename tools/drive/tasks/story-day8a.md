---
name: story-day8a
start: save:7722d878b57d
steps:
  - loaded into a room with control: world != 0 and control
  - left the start room: room != 0 and frame > 3000
forbid:
  - back at the title after loading: world == 0 and tcount == 5 and frame > 6000
---
**Frontier probe: Day 8 — Sana kidnapped → Org side street → Org Lair → Boss Org → Stella joins, from the wandering
healer's save** (`rd endsave` digest `7722d878b57d`, run `story-d7f`, slot **15**, room 340120, Arche + Sana full HP/MP,
`story` < 1310: the kidnapping is still ahead). **Loading:** at the title `tcur` boots on 2 → press `U` before Continue;
the load list opens on page 1 → `R` for page 2, then pick slot 15 by its number (not by recency). **Arche Lv7: Iron
Sword, Thick Leather, Woven Gloves (Accessory empty); Sana Lv5–6: Iron Staff, Leather Armor, Chest Sash, Green Smock.**
~1230 CP; Revival Drink, Better Herbs, Magic Candy, [Taiyaki Sweet Sticks]. Read
`/mnt/ephemeral/opensummoners/drive/agents/story-d7f/report.md` and the walkthrough's Day 8 steps 1–3 ([RD story-d7f])
first; the side-street map: `/mnt/ephemeral/opensummoners/drive/runs/story-d7f/view/map_340150_f76416.png`.

**Tools:** `rd zones R [--all]` on entering each room; `rd map R [--crop X0,Y0,X1,Y1] [--scale N]`; `rd ck R drop NAME`
(the savestate pool runs out ~21 checkpoints: drop old ones early). **Every `--score` search needs the current room in
its `--avoid`**; leave `U` out of searches that must not take doors; goal `dlg != 0` beats `script == 1`. **Never mash
`ent` through Yes/No prompts** (DRIVER.md). Do NOT allow `f2` in searches.

Next:
1. 🔑 340120 → 340130 (kidnapping cutscene) → 340125 (classmates) → back to 340120, ↑ at x 131200 → side street 340150.
   Do NOT drop into the low right alcove (a trap under the cliff, 5 Kobold-type foes); stay on the upper ledges to the
   walk-in exit at (278400, 0) → 340160. 💎 top-left ledge [Herb] if cheap. Note the traveling merchant (prices).
2. 🔑 **Org Lair** (Arche + Stella, walkthrough Day 8 step 2.1–2.4): 💎 [Tough Drop], [Org Slayer] (equip it — anti-Org);
   ⚠ left pair: left chest = poison. Gargoyle onto button; the platform + statue puzzle; fountain = save (`rd endsave`).
3. 🔑 **BOSS: Boss Org** (+ Thuggish Org, Org Lackey) → **MoH 8** → Sana rescued, Stella joins (3-member party). Stop
   there, or at Cranbal Valley's entrance if frames remain.
Save at every save point and `rd endsave <run>` after each; note each digest.

Fights: `--hold` searches; keep an avoid on each member's HP; the Revival Drink is for emergencies. Before the boss:
save at the fountain, full HP/MP. **Run `rd check` and `rd verify` when you reach ~$6 spent (mandatory)**; verify again
before `finish()` if the budget allows. Do not grind; do not read other runs' movies. Stop when the run is ~40k frames
past its start, or at a point you cannot get past after real attempts (say what blocks you, with an `rd map` PNG path).
Report: run, frames, route steps with frames, end-save digests, each character's level + equipment at the end, upgrades
taken / skipped (why), shop lists + prices seen, named state, walkthrough corrections, tool gaps.
