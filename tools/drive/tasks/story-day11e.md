---
name: story-day11e
start: story-d11d
steps:
  - in control in the bedroom: world != 0 and control
  - left the bedroom: room != 440220 and room != 0
forbid:
  - back at the title: world == 0 and tcount == 5 and frame > 2000
---
**Frontier probe: the Archmage's Tower, walkthrough Day 11 steps 5.3 → 5.6 (the library's repair book → [Glue] →
[Poton Sap] → [Rozack Polish]), from the end of run `story-d11d`** (`rd new story-d11e --from story-d11d` — the RUN,
its movie replayed: f675176 in **440220**, the headmaster's bedroom, control, leader Arche, party 7, full HP after the
bed nap). Party: **Arche** Lv15 280 (Org Slayer, Scale Armor, Leather Cape, Kitty Ears), **Sana** Lv15 209 / 268 MP,
**Stella** Lv15 237 / 206 MP (Magic Bracelet); 15987 CP; ~7 Better Herb, Best Herb ×4, Life Dew, Revival Drink ×3,
Magic Candy ×3, Recall Lamp; key items: **[Archmage's Tower Key]**. Bag 39/48. Last save: slot 7 `8b171418cd3b` (this
bed). **Read `/mnt/ephemeral/opensummoners/drive/agents/story-d11d/report.md` first** and the walkthrough's **Day 11**
step 5 (+ the [RD story-d11c] / [RD story-d11d] notes: the tower's EN-old room map).

**Route (walkthrough 5.3–5.6):**
1. 🔑 Back to the dead-end wooden door (440200 zone 3 ↑ (172800, 60800) → 440205) → key → gate → **library**: examine
   shelves; the **5th shelf to the right** = the repair book → needs [Glue], [Poton Sap], [Rozack Polish].
2. 🔑 [Glue]: "back down, left, unlock gate, tiny ledges, stairs; Arche pushes block on leftmost platform while
   Sana/Stella holds it; one member on rightmost platform, other two jump to highest → lifts rightmost to lever →
   chest". Members are parked with **F3** Follow Mode (F3 ×2 = "Stay There!", a 3-state cycle; story-d10c).
3. 🔑 [Poton Sap]: "back to area start, past stairs, right, drop pit, right stairs down; key starts platforms → cross →
   chest".
4. 🔑 [Rozack Polish]: "back, left stairs down, left, stairs down; key on lock to right; **leap of faith** far right
   off ledge → outside w/ harpies → right chest".
5. Back to the bedroom; save on the bed (`rd endsave`; all 24 slots full: overwrite the shortest play time, keep
   slots 1–7 and 24). Stop there (the next task takes 5.7, the numbered-block pot puzzle), or at ~40k frames past the
   start, or at a point you cannot get past after real attempts (say what blocks you, with an `rd map` PNG path; `rd
   map` does not draw scaffold beams / crate tops).

**Fights:** fight searches MUST avoid any member's HP dropping low — party HP in any room:
`s32(s32(s32(world+0x2784)+0x4030+4*i)+0xa64)` for i = 0..2; score fights on summed enemy HP over ≤ 8 slots (the
598-token limit). Heal from the menu only when safe: an item use is a timed cast while the game runs.

**Tools:** `rd zones R --all`; `rd map R --crop`; `rd ck R save/load/drop` (pool ~20 checkpoints). Doors fire on ↑
while standing idle (a walk leaves a ~3500 slide; a second press of the same direction within ~20 frames dashes). The
first `ent` after `f1` can be dropped: open the menu, `rd look`, then press. Chests / objects open with V facing them.
`--apply-best` with a `-ly` score lands mid-jump: hold standing states. Check every search's `found … N frames` line
BEFORE applying. Searches `-j 1`. Never step scripts through a shop list or a Yes/No / save prompt. Do NOT allow
`f2` in searches. Never wrap rd in `timeout`. Use `RD_HOST=bunker` for every rd command; one rd command at a time.

Do not grind; do not read other runs' movies. Gates cost ~35 min each at this movie length: run `rd check` + `rd verify`
ONCE (mandatory; at the end is fine). Your report goes in your FINAL MESSAGE (also try to write it to
`/mnt/ephemeral/opensummoners/drive/agents/story-d11e/report.md`). Report: run, frames, route steps with frames, end-save
digests, each character's level + equipment at the end, upgrades taken / skipped (why), named state, walkthrough
corrections (EN-old room ids, positions, puzzle solutions), tool gaps.
