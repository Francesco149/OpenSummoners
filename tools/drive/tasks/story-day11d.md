---
name: story-day11d
start: story-d11c
steps:
  - in control in the tower: world != 0 and control
  - left the tower's first room: room != 440110 and room != 0
forbid:
  - back at the title: world == 0 and tcount == 5 and frame > 2000
---
**Frontier probe: the Archmage's Tower, walkthrough Day 11 steps 5.1 → 5.2 (MoH 16 → the former headmaster → his
room: [Archmage's Tower Key], [Magic Bracelet], beds), from the end of run `story-d11c`** (`rd new story-d11d --from
story-d11c` — the RUN, its movie replayed: f639326 in **440110**, the tower's first room, control, leader Arche, party
7). Party: **Arche** Lv14 136/266 (Org Slayer, Scale Armor, Leather Cape, Kitty Ears; Blessed Earring in the bag),
**Sana** Lv14 128/201 (+Guardian Drop), **Stella** Lv14 161/228; 14079 CP; Better Herb ~11, Best Herb ×5, Life Dew,
Revival Drink ×3, Magic Candy ×4, Recall Lamp. **Heal first** (HP is low). Last save: slot 6 `31deb5360576` (Guardpost
inn) — the beds in the headmaster's room (5.2) may offer rest + save: save there (`rd endsave`; all 24 slots full:
overwrite the shortest play time, keep slots 1–6, 24). **Read
`/mnt/ephemeral/opensummoners/drive/agents/story-d11c/report.md` first** and the walkthrough's **Day 11** step 5
(+ the [RD story-d11c] note).

**Route (walkthrough 5.1 → 5.2):** "Stairs; far left [Mind Chocolate]; up; left ledges [Tough Drop]; right, stairs by
locked wall; left, stairs; right [Good Child Drop]; left, drop, stairs; climb; stairs; right → **MoH 16** (cap 19).
Wooden door = dead end for now. Up → fancy doors → former-headmaster scene. Headmaster's room: rightmost shelf 'Merrin
12/12'; far-right drawers **[Archmage's Tower Key]**; wooden door → beds (rest), closet **[Magic Bracelet]**". Tough
Drop → Arche, Good Child Drop → Sana or Stella, Magic Bracelet → whoever it suits (read the equip preview). Spend MoH /
level-ups per DRIVER.md "keep the party strong". Free bag slots before looting (cap 48: a full bag leaves the item in
the chest). 440110 zone 2 ↑ (150400, 25600) → 440120.

Stop when you hold the [Archmage's Tower Key] and have saved (the next task takes 5.3: the wooden door → the library →
the repair book), or at ~40k frames past the start, or at a point you cannot get past after real attempts (say what
blocks you, with an `rd map` PNG path; note `rd map` does not draw scaffold beams / crate tops).

**Fights:** fight searches MUST avoid any member's HP dropping low — named party HP that works in any room:
`s32(s32(s32(world+0x2784)+0x4030+4*i)+0xa64)` for i = 0..2 (story-d11c's h0..h2). Heal between fights.

**Tools:** `rd zones R --all`; `rd map R --crop`; `rd ck R save/load/drop` (the pool runs out after ~20 checkpoints:
drop old ones). Exits may need ↑. Chests open with V while standing on / facing them. Talk with V. `--apply-best` with
a `-ly` score often lands mid-jump: score or hold standing states (`lstate == 0`). Avoid a room change through a back
exit with `(room == CUR and script == 1)` in `--avoid`. Check every search's `found … N frames` line BEFORE applying.
Searches `-j 1`. `rd tap ent --until script == 0` presses one more `ent` after control returns (it swaps the leader).
Never step scripts through a shop list or a Yes/No / save prompt: read menus with `rd look` and press keys by hand.
Do NOT allow `f2` in searches. Never wrap rd in `timeout`. Be frugal with memory (one rd command at a time).

Do not grind; do not read other runs' movies. Gates cost ~35 min each at this movie length: run `rd check` + `rd verify`
ONCE (mandatory; at the end is fine). Your report goes in your FINAL MESSAGE (the orchestrator files it). Report: run,
frames, route steps with frames, end-save digests, each character's level + equipment at the end, purchases / upgrades
taken / skipped (why), named state, walkthrough corrections (EN-old room ids, positions), tool gaps.
