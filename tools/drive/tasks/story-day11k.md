---
name: story-day11k
start: story-d11j
steps:
  - in control: world != 0 and control
  - left the junction room: room != 450170 and room != 0
forbid:
  - back at the title: world == 0 and tcount == 5 and frame > 2000
---
**Frontier probe: Labyrinth of Night 7.3 → fountain 1 (save), then 7.4 if frames remain (locked door → stairs → boost
jump to lever → BOSS Skeletal Sergeant ×2 → [Pitch Black Key] + MoH 19)**, from the end of run `story-d11j` (`rd new
story-d11k --from story-d11j` — the RUN: f75582 in **450170**, the top corridor under the raised gate (lx 215816, ly
29600), control, leader Arche, Unison active). Party: **Arche Lv17** 163/288 (Org Slayer, Iron Armor, Leather Cape, Kitty
Ears), **Sana** Lv16 166/212 (220 MP), **Stella** Lv16 189/248; 12814 CP; Better Herb ~11, Best Herb ×5, Life Dew ×2,
Revival Drink ×3, Revival Drink EX, Magic Candy ×3, Mind Chocolate ×2, Recall Lamp, **[Pitch Black Key]**. Last save:
slot 11 `21d1362d3bba` (White Rose Inn — far back: **heal first, and save at fountain 1**). **Read
`/mnt/ephemeral/opensummoners/drive/agents/story-d11j/report.md` first**, then the walkthrough's Day 11 step 7 (+ the
[RD story-d11i / d11j] notes: the room chain, Unison, the 450170 exits).

**Route (walkthrough 7.3 → 7.4):**
1. Heal (menu; out of combat a Life Dew is instant). 450170 zone 2 (48000, 32000) → **450180** — guarded by 2 Skeletal
   Corporals (51252), an Org Centurion (51122) and a 51242 near the door: fight with the HP guard below.
2. 7.3: "Up past stairs, far left: ⚠ chest = **teleport trap** (DON'T open it); drop, bottom-left 2500 CP; stairs; far
   right [Broken Lamp] (Help Out item); back far left past entry stairs, jump, right → wooden door = **fountain 1**".
   At the fountain: heal + save (`rd endsave`; all 24 slots full: overwrite the shortest play time, keep slots 1–11 and
   24).
3. If frames remain — 7.4: "Right: locked door → [Pitch Black Key]; stairs down; boost jump to lever; fancy doors →
   **BOSS: Skeletal Sergeant ×2**. Stairs down → chest [Pitch Black Key] + **MoH 19**". Boss: `rd ck save` at the doors;
   full HP/MP first; a fight search scored on summed boss HP that NEVER lets any member's HP drop low.
4. Stop after MoH 19 (save at fountain 1 or the next one first if near), at ~40k frames past the start, or at a point
   you cannot get past after real attempts (say what blocks you, with an `rd map` / `rd look` PNG path). **Prefer ending
   on a save.** Assign Arche's new The Full Moon in Abilities if it helps; spend level-ups per DRIVER.md.

**Fights:** party HP `s32(s32(s32(world+0x2784)+0x4030+4*i)+0xa64)` for i = 0..2 — heal BEFORE a fight search (a follower
already near the threshold collapses the search to 1 cell); hostile HP per character slot c:
`s32(s32(c+0x750)+0x54) if u8(s32(c+0x40)) and s32(c+0x1dc) == 2 else 0`; enemy position `s32(s32(c+0x40)+4)`; many
enemies start dormant (`c+0x1d0` == 0); slot numbers move per room; ~4 proximity-filtered enemy terms fit the 598-token
limit. Leader swap `ent` + `F` = that member's quick-heal.

**Tools:** `rd zones R --all` (x = LEFT edge); `rd map R --crop` (does NOT draw 70202 planks / one-way floors);
`rd ck R save/load/drop` (pool ~30) — `ck load` / `undo`, NOT `rd rewind`; AVOID `rd strip` / `--at`. `rd search --hold`.
Searches overshoot ↑-doors: stop on distance, press ↑ after the slide stops; don't put `lstate == 0` in a goal with a
`toward` hold. `--apply-best` can apply long detours: read the path length first. Unison: sword drawn, hold X ~105
frames, release (10 MP); 2 boosts from the air; start on flat ground, not a ledge edge. Chests with V facing them.
Never step scripts through a Yes/No / save prompt. Searches `-j 1`. Do NOT allow `f2` in searches. Never wrap rd in
`timeout`. `RD_HOST=bunker` for every rd command; one at a time.

Do not grind; do not read other runs' movies. Run `rd check` + `rd verify` ONCE (mandatory; at the end is fine). Your
report goes in your FINAL MESSAGE (the orchestrator files it). Report: run, frames, route steps with frames, end-save
digests, levels + equipment at the end, upgrades taken / skipped (why), boss notes, named state, walkthrough corrections
(EN-old room ids, positions), tool gaps.
