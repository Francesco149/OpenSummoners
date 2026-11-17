---
name: story-day7e
start: save:5e899fce8699
steps:
  - loaded into a room with control: world != 0 and control
  - left the start room: room != 0 and frame > 3000
forbid:
  - back at the title after loading: world == 0 and tcount == 5 and frame > 6000
---
**Frontier probe: Chartreux → home to Tonkiness (Day 7 end) → Day 8 start, from the Dancing Piglet Inn save** (`rd endsave`
digest `5e899fce8699`, run `story-d7d`, accepted later as `story-day7d`). **Loading:** the save is slot **13** (its date
12:27 is OLDER than slot 12's — pick it by slot, not by recency); at the title `tcur` boots on 2 → press `U` before
Continue; the load list opens on page 1 → `R` for page 2. At the save: room 300240, 1003 CP, full HP, **[Taiyaki Sweet
Sticks] in hand** (Day 7's goal done). **Arche Lv7: Iron Sword, Thick Leather, Woven Gloves (Accessory empty); Sana Lv5:
Iron Staff, Leather Armor, Chest Sash, Green Smock.** Items: Herb ×3, Better Herb ×5, **Revival Drink**, Magic Candy ×3,
Mentama Drop, Merrin Card, Kitty Ears. Read `/mnt/ephemeral/opensummoners/drive/agents/story-d7d/report.md`, the
story-d7c report (the lake / boat), and the walkthrough's Day 7 steps 4–7 + Day 8 steps 1–2 ([RD] notes) first.

**Tools:** `rd zones R [--all]` on entering each room (exits may need ↑ even when labelled walk-in); `rd map R [--crop
X0,Y0,X1,Y1] [--scale N]`; `rd ck R drop NAME` frees savestates. **Never mash `ent` through Yes/No prompts** (DRIVER.md):
step choices by hand. Do NOT allow `f2` in searches.

Next:
1. 🔑 Home the way you came, reversed: Chartreux 300120 → 265110 → 265120 → 265130 → 265180 → 265190 → 265200 (the boat:
   the walkthrough says Arche pushes it / Sana fetches it — work out the return crossing; `rd map`) → 330140 (the lake;
   the boat moves toward the leader's facing) → 330135 → 330130 → 330128 → 330126/330125 → 330120 → 330110 → Loch
   Carudale 260230 → 260220 → 260215 → 260210 → 260200 (the crate) → 260190 → 260180 … → Tonkiness 210110.
   Optional while passing 330140: the [Life Dew] / [Cloth Gloves] water chests (each guarded by a Lead Merkid — kill it
   first with a `--hold` search; skip after one real attempt).
2. 🔑 Tonkiness: Sana home; talk to the parents; pajamas; save (home save) → `rd endsave`.
3. **Day 8** (if frames remain): class (Stella left without you) → exit school RIGHT (east road) → classmates, the wandering
   healer (heal / save) → Sana captured by Orgs. Stop there or at the Org Lair entrance.
Save at every inn / statue / home save and `rd endsave <run>` after each; note each digest.

Fights: `--hold` searches; keep an avoid on each member's HP; Sana's Heal Water between rooms; the Revival Drink is for
emergencies. **Run `rd check` and `rd verify` when you reach ~$6 spent (mandatory)**; verify again before `finish()` if the
budget allows. Name the state you need (`rd ram R define`). Do not grind; do not read other runs' movies. Stop when the run
is ~40k frames past its start, or at a point you cannot get past after real attempts (say what blocks you, with an `rd map`
PNG path). Report: run, frames, route steps with frames, end-save digests, each character's level + equipment at the end,
upgrades taken / skipped (why), shop lists + prices seen, named state, walkthrough corrections, tool gaps.
