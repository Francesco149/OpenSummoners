---
name: story-day11i
start: save:dfd67b2a393b
steps:
  - in control: world != 0 and control
  - left the Guardpost: room != 430120 and room != 430110 and room != 0
forbid:
  - back at the title: world == 0 and tcount == 5 and frame > 2000
---
**Frontier probe: the Guardpost → Chartreux → Tonkiness → school → the Labyrinth of Night (walkthrough Day 11 step 7 →
7.1: [Pitch Black Key] + MoH 18)**, from story-d11h's end save (`rd new story-d11i --from save:dfd67b2a393b` — slot 10,
the North Chartreux Guardpost inn 430120, full HP/MP). Party: **Arche** Lv16 (Org Slayer, Scale Armor, Leather Cape,
Kitty Ears; **Unison**: sword drawn, hold X ~105 frames, release, 10 MP — not on a ledge edge, it drifts ~3000), **Sana**
Lv16, **Stella** Lv16 (Interceptor); 19459 CP; Better Herb ×11, Best Herb ×4, Life Dew ×2, Revival Drink ×3, Magic Candy
×3, Recall Lamp ×1. **Read `/mnt/ephemeral/opensummoners/drive/agents/story-d11h/report.md` first**, then the walkthrough's
Day 11 (+ the [RD story-d11b] note: the rooms north of Chartreux; [RD story-d10a]: the Chartreux shops; [RD story-d7f]:
the school rooms) and step 7.

**Route:**
1. South: 430110 → 420180 (zone 3 x 0 is the Guardpost side) → 420170 → 420160 → 420150 (the 3.3 cave) → 420120 → 420110
   → **Chartreux**. Fights per below; don't explore 420200 / 420220.
2. Chartreux: read the weapon / armor shops' equip previews (Iron Armor 9800 for Arche was there on Day 10; a weapon
   upgrade for anyone?) and take clear upgrades, keeping ≥ 3000 CP. Then → **Tonkiness** (the way story-d11a went) →
   **school**: the Day 11 step 7 cutscene → **Labyrinth of Night**. Save before entering if there is a save point / inn
   on the way (`rd endsave`; all 24 slots full: overwrite the shortest play time, keep slots 1–10 and 24).
3. 7.1: ledge chest [Mind Chocolate]; across, stairs down; straight; next area top-left [Revival Drink EX]; drop, left
   door; boost jump (Unison) to the upper wooden door → **[Pitch Black Key] + MoH 18** (cap 21). Spend level-ups / MoH
   per DRIVER.md.
4. Stop at MoH 18 (save if a fountain / save point is near), at ~40k frames past the start, or at a point you cannot get
   past after real attempts. **If the budget runs out mid-route, end on a SAVE** (an inn / save point), as story-d11h did —
   an unsaved partial walk cannot be continued from a save.

**Fights:** fight searches MUST avoid any member's HP dropping low — party HP: `s32(s32(s32(world+0x2784)+0x4030+4*i)+0xa64)`
for i = 0..2; hostile HP per character slot c: `s32(s32(c+0x750)+0x54) if u8(s32(c+0x40)) and s32(c+0x1dc) == 2 else 0`;
enemy position `s32(s32(c+0x40)+4)`; the party's slot numbers MOVE per room (check before scoring); ~4 proximity-filtered
enemy terms fit the 598-token limit. Heal from the menu only when safe (out of combat a Life Dew is instant).

**Tools:** `rd zones R --all` (x = a zone's LEFT edge); `rd map R --crop`; `rd ck R save/load/drop` (pool ~30) — `ck load`
/ `undo`, NOT `rd rewind`; AVOID `rd strip` / `--at`. `rd search --hold`. `lstate == 0` = standing. Doors fire on ↑
while standing idle. Never step scripts through a shop list or a Yes/No / save prompt (mashing `ent` into a shop BUYS):
read menus with `rd look`, press keys by hand. Check every search's `found … N frames` line BEFORE applying. Searches
`-j 1`. Do NOT allow `f2` in searches. Never wrap rd in `timeout`. `RD_HOST=bunker` for every rd command; one at a time.

Do not grind; do not read other runs' movies. Run `rd check` + `rd verify` ONCE (mandatory; at the end is fine). Your
report goes in your FINAL MESSAGE (the orchestrator files it). Report: run, frames, route steps with frames, end-save
digests, levels + equipment at the end, purchases / upgrades taken / skipped (why), named state, walkthrough corrections
(EN-old room ids, positions), tool gaps.
