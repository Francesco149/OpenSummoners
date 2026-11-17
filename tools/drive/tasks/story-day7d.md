---
name: story-day7d
start: save:1e239cb938ff
steps:
  - loaded into a room with control: world != 0 and control
  - left the start room: room != 0 and frame > 3000
forbid:
  - back at the title after loading: world == 0 and tcount == 5 and frame > 6000
---
**Frontier probe: Chartreux City → the sweet sticks → home (Day 7), from the Dancing Piglet Inn save** (`rd endsave` digest
`1e239cb938ff`, run `story-d7c`, accepted later as `story-day7c`). At the save: room 300240 (the inn), 2503 CP, both full HP.
**Arche Lv7 (7 Marks, Combo Wide Slash), Iron Sword + Leather Armor, Armor #2 / Accessory EMPTY; Sana Lv5, Iron Staff +
Cloth Armor + Green Smock.** Items: Herb ×1, Better Herb ×3, Magic Candy ×3, Guardian Drop, Mystery Juice; **no Revival
Drink**. Read `/mnt/ephemeral/opensummoners/drive/agents/story-d7c/report.md` and the walkthrough's Day 7 steps 6–7 ([RD
story-d7c] notes) first.

**Tools:** `rd zones R [--all]` on entering each room (exits may need ↑ even when labelled walk-in); `rd map R [--crop
X0,Y0,X1,Y1] [--scale N]` when a route is unclear; `rd ck R drop NAME` frees a checkpoint's savestate (drop old ones —
the pool is finite). A search that ends `found: false` loses its best path unless you pass `--apply-best`. Do NOT allow
`f2` in searches (it burns healing items).

Next, per the walkthrough (Day 7 step 6–7):
1. **Use the unused items first:** Mystery Juice → Sana; Guardian Drop → whoever the walkthrough suggests (else Arche).
2. **Weapons store** (open Day 7): barrel 30 CP; **top drawer [Power Drop] → Arche**. Report the full stock + prices; buy the
   best affordable upgrades — Arche's empty Armor #2 / Accessory first, then her sword; Leather Armor for Sana if CP allows.
   Keep ≥ ~300 CP for the item shop.
3. **Item shop:** report stock + prices; buy a **Revival Drink** and Better Herbs if sold.
4. Candy store (200 CP capsule machine: try it ONCE if CP is spare, report the prize; the counter is sold out). Note the
   fortune teller / nun doors (past the stone archway) — talk to the fortune teller once (missed Marks?).
5. 🔑 Candy boy (far-left house, 2F) → **bakery** (city entrance; the rescued couple) → [Handmade Tart] → trade to the boy →
   **[Taiyaki Sweet Sticks]**.
6. 🔑 Home the same way (265xxx road, the boat, the Carudale Tunnels, Loch Carudale, 260190 …) to Tonkiness: Sana home,
   talk to the parents, pajamas, save. Save at every inn / statue / home save and `rd endsave <run>` after each; note each digest.

Fights: `--hold` searches (DRIVER.md); keep an avoid on each member's HP; Sana's Heal Water between rooms.
**Run `rd check` and `rd verify` when you reach ~$6 spent (mandatory)**; verify again before `finish()` if the budget allows.
Name the state you need (`rd ram R define`). Do not grind; do not read other runs' movies. Stop when the run is ~40k frames
past its start, or at a point you cannot get past after real attempts (say what blocks you, with an `rd map` PNG path).
Report: run, frames, the route steps reached with their frames, the end-save digests, each character's level + equipment
at the end and the upgrades taken / skipped (with why), shop lists + prices seen, named state, walkthrough corrections,
tool gaps.
