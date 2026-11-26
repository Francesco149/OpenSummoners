---
name: story-postgame-a
start: save:9b1a5151b929
steps:
  - in control after loading the Game Clear Data: world != 0 and control
  - left the first room: room != 0 and frame > 2000
forbid:
  - back at the title: world == 0 and tcount == 5 and frame > 4000
---
**Frontier probe: the POST-GAME — load "Game Clear Data", go back into the Labyrinth of Night, open the 4-digit code
door with the class president's number **9792**, and loot the code room** (walkthrough "POST-GAME" section), from
story-d11o's victory save (`rd new story-pga --from save:9b1a5151b929` — slot 18 "Game Clear Data"). **Read
`/mnt/ephemeral/opensummoners/drive/agents/story-d11o/report.md` first**, then the walkthrough's POST-GAME section and
the Day 11 step 7 notes ([RD story-d11i … d11k]: Tonkiness → school → 450110 … fountain 1 = 450185; note 450180's far
top-left z4 → 450400, unvisited — likely the "keep heading left" door).

**Route:** load slot 18 (title → Continue; read the slot list with `rd look`; the title cursor may start on Bonus Menu —
don't press `ent` blindly) → where it puts you (the guide: Tonkiness, state = just before Selene) → school → Labyrinth of
Night → fountain 1 (450185; save there: `rd endsave`; all 24 slots full: overwrite the shortest play time, keep slots
1–18 and 24) → "keep heading left" → the **code door**: enter **9792** (read the entry UI with `rd look`; report how it
works) → the code room: "right: spikes + [Mystery Juice]; drop, 1st left passage: ride spike traps → [Rapier] (Stella);
bottom [Stronggirl Juice]; next passage right side, drop hugging left wall → [Saint's Brooch] (Arche) + 4000 CP; drop →
1000 CP; lever teleports you back outside the door. Re-enter: 2nd left area, chest between spikes [Mind Chocolate]; hug
right twice; final lever." New monsters: Black Panther, Black Harpy. Equip Rapier → Stella, Saint's Brooch → Arche if
the previews agree. Back to fountain 1: save. Do NOT fight Selene again. Stop after the code room + save, at ~40k frames
past the start, or at a point you cannot get past after real attempts (say what blocks you, with an `rd look` PNG path).

**Fights:** party HP `s32(s32(s32(world+0x2784)+0x4030+4*i)+0xa64)` for i = 0..2 — heal BEFORE a fight search (Sana `n8`);
avoid `phmin < 40`; hostile HP per character slot c: `s32(s32(c+0x750)+0x54) if u8(s32(c+0x40)) and s32(c+0x1dc) == 2
else 0`; slot numbers move per room; add `room != R` to every in-room search's avoid; ≤ ~4 enemy terms per score.

**Tools:** `rd zones R --all` (x = LEFT edge; some doors fire only on ↑); `rd map R --crop`; `rd ck R save/load/drop` (pool
~16–30: drop old ones); `ck load` / `undo`, NOT `rd rewind`; AVOID `rd strip` / `--at`. `rd search --hold`. Short search
segments + a scripted heal phase beat one long search. Unison: sword drawn, hold X ~105 f, release. Menu message boxes
close with X. Never step scripts through a Yes/No / save prompt. Searches `-j 1`. Do NOT allow `f2` in searches. Never
wrap rd in `timeout`. `RD_HOST=bunker` for every rd command; one at a time.

Do not grind; do not read other runs' movies. Run `rd check` + `rd verify` ONCE (mandatory; at the end is fine). Your
report goes in your FINAL MESSAGE (the orchestrator files it). Report: run, frames, route steps with frames, end-save
digests, levels + equipment, the code-entry UI, the code room's layout + loot, named state, walkthrough corrections
(EN-old room ids, positions), tool gaps.
