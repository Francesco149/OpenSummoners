---
name: story-day10e
start: story-d10d
steps:
  - in control on the tower top: world != 0 and control
  - left 410230: room != 410230 and room != 0
forbid:
  - back at the title: world == 0 and tcount == 5 and frame > 2000
---
**Frontier probe: Weathervane Tower, walkthrough Day 10 step 2.8 (MoH 15 → Wind Crest 3 → home → the argument → Day
11), from the end of run `story-d10d`** (`rd new story-d10e --from story-d10d` — the RUN, its movie replayed: f460320 in
**410230**, the tower top, the Wyvern dead, control, leader Arche). Party: **Arche** Lv14 186/266 (Org Slayer, Scale
Armor, Leather Cape, **Blessed Earring**; Kitty Ears in the bag; new ability Twirling Slash, unassigned), **Sana** Lv13
121/191, **12/216 MP** (Rune Staff, Scale Armor, Chest Sash, Green Smock, Anti-Poison Ring), **Stella** Lv13 222/222,
29/183 MP (Lady's Dagger, Scale Armor, Woven Gloves, Salamander Scale); 7058 CP; Revival Drink, Magic Candy ×3, Best
Herb, Better Herbs, Recall Lamp. Last save: the 410210 fountain, slot 1, `4e700cca7925` (all 24 slots full: a new save
overwrites one — pick the oldest, keep slots 1 and 24). **Read
`/mnt/ephemeral/opensummoners/drive/agents/story-d10d/report.md` first** and the walkthrough's **Day 10** ([RD story-d10a]
… [RD story-d10d]) and **Day 11** step 1.

**Route (walkthrough 2.8 → Day 11 morning):**
1. 410230 zone 2 (108800) → 410240: "Stairs right → **MoH 15** (cap 18, 90,000)". Take it; spend MoH / level-ups per
   DRIVER.md "keep the party strong". Swap Arche's Kitty Ears back in once harpies are behind you (Atk/Def +6 vs the
   earring's Resist) — say which you chose and why.
2. "Crest too high → Colm's skirt-flip wind gets it → **[Wind Crest 3]**" — a scene; step through it with `rd tap`
   carefully (no blind `ent` mashing into Yes/No prompts).
3. "Recall Lamp / walk out → Tonkiness → home → argument → next day". The Recall Lamp is in the bag (cheapest). Save at
   the first save point you pass in Tonkiness / at home if one is offered (`rd endsave`). Stop when Day 11 starts and
   control returns (the next task takes Day 11 step 1: school, Sophia), or at ~40k frames past the start, or at a
   point you cannot get past after real attempts (say what blocks you, with an `rd map` PNG path).

Optional, only if cheap and on the way: restock healing / MP items in Tonkiness (Magic Candy / Mind Chocolate), assign
Arche's Twirling Slash if a menu pass is needed anyway.

**Tools:** `rd zones R --all`; `rd map R --crop`; `rd ck R save/load/drop` (the pool runs out after ~20 checkpoints:
drop old ones). Doors in this tower fire on ↑. Hold `--goal` searches with `--hold N` when the goal is true mid-jump.
Check every search's `found … N frames` line BEFORE applying (`--apply-best` paths of 3–10k frames cost d10d hours) —
prefer hand play on known routes. Searches `-j 1`; every `--score` search needs the current room in its `--avoid`.
`rd tap ent --until script == 0` presses one more `ent` after control returns (it swaps the leader): stop earlier or
undo. Never `rd tap ent` through dialogue that may end in a Yes/No or save prompt; do NOT allow `f2` in searches.
`rd text` does not show menu lists — read menus with `rd look`.

Do not grind; do not read other runs' movies. **Run `rd check` and `rd verify` at about two thirds of your effort
(mandatory)**; verify again before you finish if you can. Report: run, frames, route steps with frames, end-save
digests, each character's level + equipment at the end, upgrades taken / skipped (why), named state, walkthrough
corrections, tool gaps.
