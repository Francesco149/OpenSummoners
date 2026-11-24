---
name: story-day11c
start: story-d11b
steps:
  - in control at the Guardpost inn: world != 0 and control
  - left the inn: room != 430120 and room != 0
forbid:
  - back at the title: world == 0 and tcount == 5 and frame > 2000
---
**Frontier probe: Day 11, walkthrough step 3.3 (rest) → 3.4 (the Grand Gourmand + [Ultimate Bread] → the secret path)
→ 4 (Guardpost loot) → 5 start (the Archmage's Tower entrance), from the end of run `story-d11b`** (`rd new story-d11c
--from story-d11b` — the RUN, its movie replayed: f608262 in **430120**, the Mountain Cat Inn at the North Chartreux
Guardpost, rested, control, leader Arche, party 7). Party: **Arche** Lv14 266 (Org Slayer, Scale Armor, Leather Cape,
Kitty Ears; Blessed Earring in the bag), **Sana** Lv14 201, **Stella** Lv14 228; 12726 CP; Better Herb ~14, Best Herb,
Revival Drink ×3, Magic Candy ×4, Recall Lamp; story items incl. **Ultimate Bread (not yet given)**, Cleitcaithream.
Last save: slot 6 `31deb5360576` (this inn). All 24 slots full: overwrite the shortest play time; keep slots 1–6, 24.
**Read `/mnt/ephemeral/opensummoners/drive/agents/story-d11b/report.md` first** and the walkthrough's **Day 11** steps
3–5 (+ the [RD story-d11a] / [RD story-d11b] notes — the EN-old room map north of Chartreux).

**Route:**
1. Shop here first if worth it: the **Magic Bracelet** (11000) — read its stats in the shop preview (`rd look`) and
   compare with each member's accessory; buy it only if it is a clear upgrade for someone and ~1500 CP stays for
   healing. Say what you chose.
2. Find the **Grand Gourmand**: 430110 → 420180 (↑ at the x 0 side from 430110's right) → try **420180 zone 2 (top
   right, (252800, 6400)) → 420220** first; else 420160's top-left exit → 420200. Use `rd zones R --all` + `rd map R`
   on each new room. On the way: the 3.3 "quick jumps on wooden crates to top → [Guardian Drop], [Life Dew]" (Guardian
   Drop → Sana or Stella). Give the Gourmand the **[Ultimate Bread]** → the secret path between two rocks.
3. 4. (💎, only if cheap) the Guardpost barrel [Power Drop] — story-d11b could not open barrel 70302 with X / V /
   slashes: try other actions (push? Stella's fire? the 2F rooms of the inn per the walkthrough), ≤ 2k frames.
4. Through the secret path to the **Archmage's Tower** entrance; stop when inside its first room in control (the next
   task takes 5.1), or at ~40k frames past the start, or at a point you cannot get past after real attempts (say what
   blocks you, with an `rd map` PNG path). Save at an inn / statue if one is passed (`rd endsave`).

**Fights:** fight searches MUST avoid any member's HP dropping low (story-d11b applied a cell with Sana dead): avoid
on every member (`s32(s32(s32(world+0x1160+4*SLOT)+0x750)+0x54)` per party slot) — not the leader alone. Heal
between fights.

**Tools:** `rd zones R --all`; `rd map R --crop`; `rd ck R save/load/drop` (the pool runs out after ~20 checkpoints:
drop old ones). Exits labelled "walk/up" may need ↑. Chests open with V while standing on their x. An exit held by
foes needs them dead. Search goals: `room == TARGET` with the other rooms in `--avoid`. Check every search's `found … N
frames` line BEFORE applying. Searches `-j 1`; every `--score` search needs the current room in its `--avoid`. `rd tap
ent --until script == 0` presses one more `ent` after control returns (it swaps the leader). Never `rd tap ent` /
step scripts through a shop list or a Yes/No / save prompt (story-d11b bought 4 herbs that way): read menus with `rd
look` and press keys by hand there. Do NOT allow `f2` in searches. Never wrap rd in `timeout`. Be frugal with memory
(one rd command at a time; no nix-shell side tools).

Do not grind; do not read other runs' movies. Gates cost ~30 min each at this movie length: run `rd check` + `rd verify`
ONCE (mandatory; at the end is fine). Your report goes in your FINAL MESSAGE (the orchestrator files it; subagents may
not be allowed to write report.md). Report: run, frames, route steps with frames, end-save digests, each character's
level + equipment at the end, purchases / upgrades taken / skipped (why), named state, walkthrough corrections (EN-old
room ids, positions), tool gaps.
