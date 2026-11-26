---
name: story-day11h
start: save:e65ce3a856fa
steps:
  - in control: world != 0 and control
  - left the tower: room < 440000 or room >= 450000
forbid:
  - back at the title: world == 0 and tcount == 5 and frame > 2000
---
**Frontier probe: leave the Archmage's Tower → school → the Labyrinth of Night, walkthrough Day 11 step 7 → 7.1
([Pitch Black Key] + MoH 18)**, from story-d11g's end save (`rd new story-d11h --from save:e65ce3a856fa` — slot 9, the
440220 bed after MoH 17). Party: **Arche** Lv16 (Org Slayer, Scale Armor, Leather Cape, Kitty Ears; **Unison + Wind
Thrust** learned: sword drawn, hold X ~105 frames, release, 10 MP; C in the air chains boost jumps), **Sana** Lv15 (Rune
Staff, Scale Armor, Chest Sash, Green Smock, Anti-Poison Ring), **Stella** Lv15 (Lady's Dagger, Scale Armor, Woven Gloves,
Magic Bracelet); 18856 CP; ~5 Better Herb, Best Herb ×4, Life Dew, Revival Drink ×3, Magic Candy ×3, **Recall Lamp**.
**Read `/mnt/ephemeral/opensummoners/drive/agents/story-d11g/report.md` first**, then the walkthrough's Day 11 steps 6–7
(+ the [RD story-d11c … d11g] notes: the tower map, the way out) and the Tonkiness / school notes from earlier days.

**Route:**
1. Out of the tower (the Recall Lamp from the menu may return you to town — read what it offers with `rd look`; else
   walk out the way story-d11c came in) → **Tonkiness**. Skip the optional Unison backtrack (walkthrough step 6).
2. Prep in Tonkiness (DRIVER.md "keep the party strong"): heal items up to ~10 Better Herb + a couple of Revival Drinks;
   read the weapon / armor shops' equip previews and take a clear upgrade if CP allows (keep ≥ 3000 CP). Save at the inn
   or a save point (`rd endsave`; all 24 slots full: overwrite the shortest play time, keep slots 1–9 and 24).
3. 🔑 Go to **school** → cutscene: darkness ("Dark World") → **Labyrinth of Night**. 7.1: ledge chest [Mind Chocolate];
   across, stairs down; straight; next area top-left [Revival Drink EX]; drop, left door; boost jump (Unison) to the upper
   wooden door → **[Pitch Black Key] + MoH 18** (cap 21). Spend level-ups / MoH per DRIVER.md.
4. Save at a fountain if the labyrinth has one near (fountain 1 is in 7.3 — do not go that far unless frames remain).
   Stop at MoH 18, at ~40k frames past the start, or at a point you cannot get past after real attempts (say what blocks
   you, with an `rd map` / `rd look` PNG path).

**Fights:** fight searches MUST avoid any member's HP dropping low — party HP in any room:
`s32(s32(s32(world+0x2784)+0x4030+4*i)+0xa64)` for i = 0..2; score fights on summed enemy HP over ≤ 8 slots (the
598-token limit). Heal from the menu only when safe (an item use is a timed cast; its closing `ent` can swap the leader).
⚠ The labyrinth has teleport-trap chests (7.3 / 7.5) — not in this leg's range.

**Tools:** `rd zones R --all`; `rd map R --crop`; `rd ck R save/load/drop` (pool ~30; full ⇒ search prints only an
error) — use `ck load` / `undo`, NOT `rd rewind`; AVOID `rd strip` / `--at` (replay from boot, the session idles out and
is rebuilt). `rd search --hold` keeps a goal reached mid-action from being undone. Hold standing states with
`lstate == 0`. Follow Mode: F1 → Party Commands. Doors fire on ↑ while standing idle. Objects with V facing them. Check
every search's `found … N frames` line BEFORE applying. Searches `-j 1`. Never step scripts through a shop list or a
Yes/No / save prompt: read menus with `rd look`, press keys by hand. Do NOT allow `f2` in searches. Never wrap rd in
`timeout`. `RD_HOST=bunker` for every rd command; one rd command at a time.

Do not grind; do not read other runs' movies. Run `rd check` + `rd verify` ONCE (mandatory; at the end is fine). Your
report goes in your FINAL MESSAGE (the orchestrator files it). Report: run, frames, route steps with frames, end-save
digests, levels + equipment at the end, purchases / upgrades taken / skipped (why), named state, walkthrough corrections
(EN-old room ids, positions), tool gaps.
