---
name: story-day11m
start: save:cf1d4316f966
steps:
  - in control: world != 0 and control
  - left the fountain room: room != 450225 and room != 0
forbid:
  - back at the title: world == 0 and tcount == 5 and frame > 2000
---
**Frontier probe: Labyrinth of Night 7.6 rest → 7.7 (450240: [Celebrity Ring] (Stella), 💎 the skeleton [White Iron Ore],
push the right gargoyle left → fancy doors; then the doors: Sana's water chest [Tough Drop], boat, [Mystery Juice],
falling spikes, [Star Rod] (Sana) → the 4-way junction)**, from story-d11l's end save (`rd new story-d11m --from
save:cf1d4316f966` — slot 15, **fountain 2** = 450225, full HP/MP). Party all **Lv18** (cap 21): **Arche** (Org Slayer,
Iron Armor, Leather Cape, Kitty Ears; Unison), **Sana** (Heal Water II = numpad n8, 6 MP: near-free healing; Mermaid
Robe n6), **Stella** (Magician's Outfit, Magic Bracelet); 23326 CP; ~6 Better Herb, Best Herb ×3, Revival Drink ×2 + EX,
Magic Candy ×3, Recall Lamp ×2 (in the labyrinth it goes to Tonkiness — don't); **[Pitch Black Key] ×1**. **Read
`/mnt/ephemeral/opensummoners/drive/agents/story-d11l/report.md` first**, then the walkthrough's Day 11 step 7 (+ the
[RD story-d11j / d11k / d11l] notes: the labyrinth's room chain).

**Route:** 450225 → 450220 → zone 2 (67200, 115200) → 450230 → **z2 (432000, 35200)** → **450240** (unvisited). Walkthrough
7.6: "exit top right; drop, right; top right past up-stairs → boost → [Celebrity Ring] (Stella). Bottom right: push
right gargoyle left → fancy doors appear. 💎 first: far left of this area, boost up, examine skeleton → [White Iron Ore]
(Help Out)." (The 450220 statues were background; the pushable pair should be here — `rd zones --all` + object lists
first.) Then 7.7: "Doors: Sana water chest [Tough Drop]; boat; wooden door; right [Mystery Juice]; ⚠ falling spike traps
after stone blocks; unlock end → right [Star Rod] (Sana); top right stairs down → **4-way junction**." Equip Celebrity
Ring → Stella, Star Rod → Sana if the preview agrees; Tough Drop → whoever it suits. Save at a fountain / statue whenever
you pass one (`rd endsave`; all 24 slots full: overwrite the shortest play time, keep slots 1–15 and 24). Stop at the
4-way junction, at ~40k frames past the start, or at a point you cannot get past after real attempts (say what blocks
you, with an `rd map` / `rd look` PNG path). **End on a save** if any save point is in reach; else say so.

**Fights:** party HP `s32(s32(s32(world+0x2784)+0x4030+4*i)+0xa64)` for i = 0..2 — heal BEFORE a fight search (Sana n8);
avoid `phmin < 40`; hostile HP per character slot c: `s32(s32(c+0x750)+0x54) if u8(s32(c+0x40)) and s32(c+0x1dc) == 2
else 0`; dormant `c+0x1d0` == 0; slot numbers move per room; add `room != R` to every in-room search's avoid.
Auto-guard (`lstate` 5 with a foe near) blocks V: clear foes before chests.

**Tools:** `rd zones R --all` (x = LEFT edge; some doors need ↑ while standing ON them); `rd map R --crop` (no 70202 /
70210 lifts, planks, blocks); `rd ck R save/load/drop` (`rd stop` clears a wedged pool); `ck load` / `undo`, NOT
`rd rewind`; AVOID `rd strip` / `--at`. `rd search --hold`. A "1 cell only" search = a doomed start state (plank about to
drop, mid-air, auto-guard) — fix the start. Unison: sword drawn, hold X ~105 frames, release (10 MP), ~2000 frames, 2 air
boosts, flat ground. Menu message boxes close with X. Never step scripts through a Yes/No / save prompt. Searches
`-j 1`. Do NOT allow `f2` in searches. Never wrap rd in `timeout`. `RD_HOST=bunker` for every rd command; one at a time.

Do not grind; do not read other runs' movies. Run `rd check` + `rd verify` ONCE (mandatory; at the end is fine). Your
report goes in your FINAL MESSAGE (the orchestrator files it). Report: run, frames, route steps with frames, end-save
digests, levels + equipment at the end, upgrades taken / skipped (why), named state, walkthrough corrections (EN-old
room ids, positions), tool gaps.
