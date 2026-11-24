---
name: story-day11b
start: story-d11a
steps:
  - in control in Chartreux: world != 0 and control
  - left the inn: room != 300240 and room != 0
forbid:
  - back at the title: world == 0 and tcount == 5 and frame > 2000
---
**Frontier probe: Day 11, walkthrough step 3 (north of Chartreux: 3.1 optional cave → 3.2 the flower field →
[Cleitcaithream] → 3.3 the next cave → 3.4 the Grand Gourmand + [Ultimate Bread] → the secret path), from the end of
run `story-d11a`** (`rd new story-d11b --from story-d11a` — the RUN, its movie replayed: f546689 in **300240**, the
Dancing Piglet Inn, Chartreux, control, leader Arche, party 7, HP / MP full). Party: **Arche** Lv14 266 (Org Slayer, Scale
Armor, Leather Cape, Kitty Ears; **Blessed Earring in the bag**), **Sana** Lv13 191, **Stella** Lv13 222 (gear as
story-d10e); 9104 CP; Recall Lamp, Revival Drink ×3, Magic Candy ×5, Better Herb ×11, Best Herb; story: Sophia's Letter,
Ultimate Bread. Last save: slot 5 `704e90e24d19` (Chartreux inn). All 24 slots full: overwrite the save with the least
play time; keep slots 1–5 and 24. **Read `/mnt/ephemeral/opensummoners/drive/agents/story-d11a/report.md` first** and
the walkthrough's **Day 11** steps 3–4 (+ the [RD story-d11a] note).

**Route:**
1. On the way out: Chartreux weapons store 300220 — compare Bronze Gauntlets (2800) vs Arche's Leather Cape and any
   girls' upgrades; buy what helps within ~7000 CP (keep ~2000 for healing). Iron Armor 9800 is out of reach.
2. Exit Chartreux **LEFT of the candy store** (north). Young Harpy / Harpy appear: put the **Blessed Earring** on Arche
   if confusion bites (say what you chose).
3. 3.1 (💎 optional, the statue-on-a-boat cave → 1000 CP + [Mystery Juice]): skip if it costs > ~8k frames.
   **3.2:** the next area → the flower-field event → **[Cleitcaithream]** (Help Out item; needs all 3 members).
   **3.3:** the next cave: ⚠ ceiling spike trap; far left 850 CP; stairs, left, bottom-left ledge [Revival Drink]; ledges
   right; quick jumps on wooden crates to the top → [Guardian Drop] + [Life Dew] (Guardian Drop → Sana or Stella).
   **3.4:** the **Grand Gourmand**: give [Ultimate Bread] → the secret path between two rocks.
4. Stop when the secret path opens and control returns (the next task takes step 4 — the guardpost — and step 5, the
   Archmage's Tower), or at ~40k frames past the start, or at a point you cannot get past after real attempts (say what
   blocks you, with an `rd map` PNG path). Save at a statue / inn if one is passed (`rd endsave`).

**Tools:** `rd zones R --all`; `rd map R --crop`; `rd ck R save/load/drop` (the pool runs out after ~20 checkpoints:
drop old ones). An exit held by foes needs the foes dead (score on their summed HP — story-d11a's `fight.sh` in the
scratchpad). Search goals: `room == TARGET` with the other rooms in `--avoid`, not `room != CUR`. Check every
search's `found … N frames` line BEFORE applying. Searches `-j 1`; every `--score` search needs the current room in its
`--avoid`. `rd tap ent --until script == 0` presses one more `ent` after control returns (it swaps the leader). Never
`rd tap ent` through dialogue that may end in a Yes/No, shop or save prompt; step dialogue one line at a time. Do NOT
allow `f2` in searches. `rd text` does not show menu lists — read menus with `rd look`. Never wrap rd in `timeout`.

Do not grind; do not read other runs' movies. Gates cost ~30 min each at this movie length: run `rd check` + `rd verify`
ONCE, at about two thirds of your effort or at the end (mandatory; say which). Report: run, frames, route steps with
frames, end-save digests, each character's level + equipment at the end, purchases / upgrades taken / skipped (why),
named state, walkthrough corrections (EN-old room ids, positions), tool gaps.
