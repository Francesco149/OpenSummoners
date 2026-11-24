---
name: story-day11a
start: story-d10e
steps:
  - in control on Day 11: world != 0 and control
  - left the school classroom: room != 230140 and room != 0
forbid:
  - back at the title: world == 0 and tcount == 5 and frame > 2000
---
**Frontier probe: Day 11, walkthrough steps 1–3.2 (Sophia's Letter → Barness Mayor [Ultimate Wheat] → Chartreux bakery
[Ultimate Bread] → north of Chartreux: the first cave (optional) → the flower field event [Cleitcaithream]), from the
end of run `story-d10e`** (`rd new story-d11a --from story-d10e` — the RUN, its movie replayed: f491227 in **230140**,
the school classroom on Day 11, control, leader Arche, party 7 = all three; `story` 1600). Party: **Arche** Lv14 266/266
(Org Slayer, Scale Armor, Leather Cape, Kitty Ears; Blessed Earring in the bag), **Sana** Lv13 191 (Rune Staff, Scale
Armor, Chest Sash, Green Smock, Anti-Poison Ring), **Stella** Lv13 222 (Lady's Dagger, Scale Armor, Woven Gloves,
Salamander Scale); 7089 CP (cap 80000); Revival Drink ×2, Magic Candy ×3, Best Herb, Better Herbs. No Recall Lamp. Last
save: slot 2, `f0048b995ad1` (all 24 slots full: overwrite the save with the least play time; keep slots 1, 2, 4, 24).
**Read `/mnt/ephemeral/opensummoners/drive/agents/story-d10e/report.md` first** and the walkthrough's **Day 11** steps
1–4 (+ the [RD story-d10e] note above it) and the Chartreux / Barness notes from Days 5–9 ([RD …] lines: rooms, shops).

**Route:**
1. School: ask **Sophia** (the staff room) about the stone → [Sophia's Letter]. "Chiffon: return the Weathervane key"
   if she is on the way (note what happens).
2. **Barness Mayor** (windmills turning): return the key → [Ultimate Wheat]. → **Chartreux bakery** man → [Ultimate
   Bread]. Gear: 7089 CP — check the Chartreux weapons / armor stores (300220 …) for upgrades over Org Slayer / Scale
   Armor / the girls' weapons and buy what fits the budget (DRIVER.md "keep the party strong"; keep ~1000 CP for
   healing); buy Magic Candy / Mind Chocolate and a **Recall Lamp** at the Chartreux item store. Save at an inn on the
   way (`rd endsave`).
3. Exit Chartreux LEFT of the candy store (north). 3.1 (💎 optional first cave: the statue-on-a-boat puzzle — skip if it
   costs > ~6k frames) → **3.2 next area → the flower-field event → [Cleitcaithream]**. Stop after that event returns
   control (the next task takes 3.3 onward), or at ~40k frames past the start, or at a point you cannot get past after
   real attempts (say what blocks you, with an `rd map` PNG path).

Swap the Blessed Earring back onto Arche before the harpy fields north of Chartreux if confusion becomes a problem
(Young Harpy / Harpy appear there); say what you chose.

**Tools:** `rd zones R --all`; `rd map R --crop`; `rd ck R save/load/drop` (the pool runs out after ~20 checkpoints:
drop old ones). Search goals: use `room == TARGET` with the other rooms in `--avoid`, not `room != CUR`. Check every
search's `found … N frames` line BEFORE applying. Searches `-j 1`; every `--score` search needs the current room in its
`--avoid`. `rd tap ent --until script == 0` presses one more `ent` after control returns (it swaps the leader). Never
`rd tap ent` through dialogue that may end in a Yes/No, shop or save prompt (story-d7d paid a fortune teller that way);
step dialogue one line at a time (story-d10e's `step.sh` in the scratchpad is a model). Do NOT allow `f2` in searches.
`rd text` does not show menu lists — read menus with `rd look`.

Do not grind; do not read other runs' movies. **Run `rd check` and `rd verify` at about two thirds of your effort
(mandatory — d10e skipped it)**; verify again before you finish if you can. Report: run, frames, route steps with
frames, end-save digests, each character's level + equipment at the end, purchases / upgrades taken / skipped (why),
named state, walkthrough corrections (EN-old room ids, NPC positions), tool gaps.
