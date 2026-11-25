---
name: story-day11f
start: story-d11e
steps:
  - in control: world != 0 and control
  - left the start room: room != 440130 and room != 0
forbid:
  - back at the title: world == 0 and tcount == 5 and frame > 2000
---
**Frontier probe: the Archmage's Tower, walkthrough Day 11 step 5.7 (save in the bedroom → repair the pot = the numbered
block puzzle I–IX → seal broken: Unison / boost jump), from the end of run `story-d11e`** (`rd new story-d11f --from
story-d11e` — the RUN, its movie replayed: f715348 in **440130** lower floor (arrived from 440140), control, leader Arche).
Party: **Arche** Lv15 261/280, **Sana** Lv15 145/209, **Stella** Lv15 158/237 (gear as story-d11d); 18003 CP; ~5 Better Herb,
Best Herb ×4, Life Dew, Revival Drink ×3, Magic Candy ×3, Recall Lamp; key items: Tower Key, **[Glue] [Poton Sap] [Rozack
Polish]**. Last save: slot 7 `8b171418cd3b` (bedroom bed, BEFORE the ingredients). **Read
`/mnt/ephemeral/opensummoners/drive/agents/story-d11e/report.md` first** and the walkthrough's **Day 11** step 5 (+ the
[RD story-d11c/d/e] notes: the tower's EN-old room map).

**Route:**
1. Back up to **440220** (the bedroom: 440130 → 440150 → … → 440200 → zone 2 → 440210 → 440220; see the notes). **Save on
   the bed first** (`rd endsave`; all 24 slots full: overwrite the shortest play time, keep slots 1–7 and 24).
2. 🔑 Repair the pot (the headmaster / the broken pot in 440210 or the library 440205 — find where the repair starts) →
   **numbered block puzzle I–IX**. Walkthrough [DK] solution (paraphrased): VII far left; IX off left edge; VIII off right
   edge; V off right edge onto VIII; push VIII+V stack left to cover left half of VII/IV/I column; Stella burns vines → VI
   drops onto the stack's top-right; push stack to IX; place VI; place VII; IV off left edge then off right edge → its spot;
   burn vines AROUND (not under) II, push II slightly left, burn under it → falls to spot (nudge); I off right edge → spot;
   member on right platform (down), push III onto it; both girls on left platform → III level w/ left ledge; push III onto
   ledge; girls move to right platform; push III off left edge → spot. **Breaking a block resets; a lever neatens blocks.**
   Use `rd ck save` before every irreversible push; `rd look` often; describe the board's layout (block ids, positions) in
   the report — it is the first time we see it.
3. Talk to the magician → events → **seal broken: Unison** (boost jump). Save again on the bed (`rd endsave`).
4. If frames remain: 5.8 roof stairs (440220 zone 2 ↑ (19200, 35200) → 440230) → **MoH 17** (cap 20) with the boost jump;
   left side plank scaffolding → boost to chest [Memory Photo 7]. Stop there, at ~40k frames past the start, or at a point
   you cannot get past after real attempts (say what blocks you, with an `rd map` / `rd look` PNG path).

**Fights:** fight searches MUST avoid any member's HP dropping low — party HP in any room:
`s32(s32(s32(world+0x2784)+0x4030+4*i)+0xa64)` for i = 0..2; score fights on summed enemy HP over ≤ 8 slots (the
598-token limit). Heal from the menu only when safe: an item use is a timed cast while the game runs (and its closing `ent`
can swap the leader).

**Tools:** `rd zones R --all`; `rd map R --crop`; `rd ck R save/load/drop` (pool ~20 checkpoints) — use `ck load` / `undo`,
NOT `rd rewind` (rebuilds the whole session: ~33 min). Doors fire on ↑ while standing idle. Hold standing states with
`lstate == 0` (not `lpose`/`ly` alone). F3 presses can be dropped: use F1 → Party Commands → Follow Mode. Chests / objects /
levers open with V facing them. Check every search's `found … N frames` line BEFORE applying. Searches `-j 1`. Never step
scripts through a shop list or a Yes/No / save prompt. Do NOT allow `f2` in searches. Never wrap rd in `timeout`. Use
`RD_HOST=bunker` for every rd command; one rd command at a time.

Do not grind; do not read other runs' movies. Gates cost ~40 min each at this movie length: run `rd check` + `rd verify`
ONCE (mandatory; at the end is fine). Your report goes in your FINAL MESSAGE (also try to write it to
`/mnt/ephemeral/opensummoners/drive/agents/story-d11f/report.md`). Report: run, frames, route steps with frames, end-save
digests, each character's level + equipment at the end, upgrades taken / skipped (why), named state, the pot puzzle's
board + the solution that worked, walkthrough corrections (EN-old room ids, positions), tool gaps.
