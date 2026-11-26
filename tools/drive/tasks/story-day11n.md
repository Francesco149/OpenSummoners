---
name: story-day11n
start: story-d11m
steps:
  - in control: world != 0 and control
  - left the Star Rod room: room != 450260 and room != 0
forbid:
  - back at the title: world == 0 and tcount == 5 and frame > 2000
---
**Frontier probe: Labyrinth of Night 7.7 end → the 4-way junction → 7.9 (bottom-right path: **MoH 20** + **fountain 3**,
SAVE) → 7.8 if frames remain (bottom-left crate / platform puzzle → [Pitch Black Key])**, from the end of run
`story-d11m` (`rd new story-d11n --from story-d11m` — the RUN: f37295 in **450260**, bottom right at the opened Star Rod
chest, full HP, control, leader Arche). Party all **Lv18** (cap 21): **Arche** 303 HP, **15/57 MP** (Org Slayer, Iron
Armor, Leather Cape, Kitty Ears; Unison 10 MP — top up MP: Magic Candy ×3), **Sana** 236 HP / 81 MP (Star Rod; Heal Water
II = `n8`, hold X ~75 f), **Stella** 265 HP (Magician's Outfit, Celebrity Ring); 27817 CP; Better Herb ×5, Best Herb ×2,
Revival Drink ×2 + EX, Mind Chocolate, Recall Lamp ×2 (goes to Tonkiness — don't). No Pitch Black Key held. Last save:
slot 15 `cf1d4316f966` (fountain 2, far back). **Read `/mnt/ephemeral/opensummoners/drive/agents/story-d11m/report.md`
first** (the way to 450260's exit, the guards on it), then the walkthrough's Day 11 step 7 (+ the [RD story-d11j … d11m]
notes).

**Route:**
1. 450260 → exit z2 (275200, 32000) top right: back left to the middle shaft (x ≈ 96000–105600), up, right along the top
   corridor (trap gates 70230 at 154200 / 218200 hold 2 Ice Dragons ~680 HP; 2 × 51252 + 51083 on the corridor's left;
   51241 + 2 Centurions near the exit) → 450270 → "top right stairs down → **4-way junction**".
2. **7.9 first (the save):** "Bottom-right path → **MoH 20** (cap 23) + **fountain 3**". Heal + save at fountain 3
   (`rd endsave`; all 24 slots full: overwrite the shortest play time, keep slots 1–15 and 24).
3. If frames remain — 7.8: "Bottom-left path → wooden door: crate/platform puzzle (boost Arche to crate; member on right
   platform raises left; push crate on; member off; push crate off; member on right platform; push crate onto right
   platform; Arche boosts onto left platform; Sana+Stella join left; Arche off, push crate onto button) → [Pitch Black
   Key]". Followers are parked with F1 → Party Commands → Follow Mode ('Stay There!'); a 'Stay There!' follower on a
   moving platform may hover and weigh nothing until moved (story-d11g). Back to fountain 3: save again.
4. Stop after the 7.8 key + save, at ~40k frames past the start, or at a point you cannot get past after real attempts
   (say what blocks you, with an `rd map` / `rd look` PNG path). Do NOT go on to 7.10 (the final boss's approach).
   **End on a save.**

**Fights:** party HP `s32(s32(s32(world+0x2784)+0x4030+4*i)+0xa64)` for i = 0..2 — heal BEFORE a fight search (Sana
`n8`); avoid `phmin < 40`; hostile HP per character slot c: `s32(s32(c+0x750)+0x54) if u8(s32(c+0x40)) and s32(c+0x1dc)
== 2 else 0`; dormant `c+0x1d0` == 0; slot numbers move per room; add `room != R` to every in-room search's avoid. Trap
gates 70230 close on approach and open when their monsters die. Auto-guard (`lstate` 5, foe near) blocks V.

**Tools:** `rd zones R --all` (x = LEFT edge; some doors need ↑ while standing ON them); `rd map R --crop` (no 70202 /
70210 lifts, planks, blocks); `rd ck R save/load/drop` (`rd stop` clears a wedged pool); `ck load` / `undo`, NOT
`rd rewind`; AVOID `rd strip` / `--at`. `rd search --hold`. A "1 cell only" search = a doomed start state. Unison: sword
drawn, hold X ~105 frames, release (10 MP), ~2000 frames, 2 air boosts, flat ground; its momentum makes repeated
`R*8 .*10` a dash. `rd tap ent --until script == 0` swaps the leader if nothing opened — check first. Menu message boxes
close with X. Never step scripts through a Yes/No / save prompt. Searches `-j 1`. Do NOT allow `f2` in searches. Never
wrap rd in `timeout`. `RD_HOST=bunker` for every rd command; one at a time.

Do not grind; do not read other runs' movies. Run `rd check` + `rd verify` ONCE (mandatory; at the end is fine). Your
report goes in your FINAL MESSAGE (the orchestrator files it). Report: run, frames, route steps with frames, end-save
digests, levels + equipment at the end, upgrades taken / skipped (why), the crate puzzle's layout + solution, named
state, walkthrough corrections (EN-old room ids, positions), tool gaps.
