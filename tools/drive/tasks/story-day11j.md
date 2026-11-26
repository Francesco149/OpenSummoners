---
name: story-day11j
start: story-d11i
steps:
  - in control in the labyrinth: world != 0 and control
  - left the entrance hall: room != 450110 and room != 0
forbid:
  - back at the title: world == 0 and tcount == 5 and frame > 2000
---
**Frontier probe: Labyrinth of Night 7.1 → 7.2 ([Mind Chocolate], [Revival Drink EX], the boost jump → [Pitch Black Key]
+ MoH 18; then the water cavern [Guardian Drop] / [Best Herb])**, from the end of run `story-d11i` (`rd new story-d11j
--from story-d11i` — the RUN: f43827 in **450110**, the entrance hall, control, leader Arche). Party Lv16 ×3 (Arche
237/281 — Org Slayer, **Iron Armor**, Leather Cape, Kitty Ears; **Unison**: sword drawn, hold X ~105 frames, release,
10 MP, not on a ledge edge; C in the air chains boosts), Sana 212/212, Stella 240/248; 10755 CP; Better Herb ×11, Best
Herb ×4, Life Dew ×2, Revival Drink ×3, Magic Candy ×3, Recall Lamp ×1. Last save: slot 11 `21d1362d3bba` (White Rose
Inn — far back). **Read `/mnt/ephemeral/opensummoners/drive/agents/story-d11i/report.md` first**, then the walkthrough's
Day 11 step 7 (+ the [RD story-d11g / d11i] notes: Unison, the 450110 exits).

**Route (walkthrough 7.1–7.2):** "Ledge chest [Mind Chocolate]; across, stairs down; straight; next area top-left
[Revival Drink EX]; drop, left door; boost jump to upper wooden door → [Pitch Black Key] + **MoH 18** (cap 21, 120,000).
Back; right, stairs down; far right wooden door → water cavern: Sana / Mermaid Robe → [Guardian Drop], [Best Herb]."
Spend MoH / level-ups per DRIVER.md (Guardian Drop → whoever it suits). Free bag slots before looting (cap 48).
⚠ 7.3 has a teleport-trap chest ("up past stairs, far left") and fountain 1 — go no further than the water cavern unless
frames remain, and then only to **fountain 1** (save there: `rd endsave`; all 24 slots full: overwrite the shortest play
time, keep slots 1–11 and 24). Stop at fountain 1, at ~40k frames past the start, or at a point you cannot get past
after real attempts (say what blocks you, with an `rd map` / `rd look` PNG path). Ending on a save is best; if not,
the next leg continues from this RUN.

**Fights:** fight searches MUST avoid any member's HP dropping low — party HP: `s32(s32(s32(world+0x2784)+0x4030+4*i)+0xa64)`
for i = 0..2; hostile HP per character slot c: `s32(s32(c+0x750)+0x54) if u8(s32(c+0x40)) and s32(c+0x1dc) == 2 else 0`;
enemy position `s32(s32(c+0x40)+4)`; the party's slot numbers move per room; ~4 proximity-filtered enemy terms fit the
598-token limit. New monsters here: read them before fighting. Heal from the menu when safe.

**Tools:** `rd zones R --all` (x = LEFT edge); `rd map R --crop`; `rd ck R save/load/drop` (pool ~30: drop old ones
early) — `ck load` / `undo`, NOT `rd rewind`; AVOID `rd strip` / `--at`. `rd search --hold`. Searches overshoot ↑-doors:
stop on distance, then press ↑ after the slide stops (waits on `lstate == 0` with a `toward` hold can stall). Objects /
chests with V facing them. Never step scripts through a Yes/No / save prompt. Check every search's `found … N frames`
line BEFORE applying. Searches `-j 1`. Do NOT allow `f2` in searches. Never wrap rd in `timeout`. `RD_HOST=bunker` for
every rd command; one at a time.

Do not grind; do not read other runs' movies. Run `rd check` + `rd verify` ONCE (mandatory; at the end is fine). Your
report goes in your FINAL MESSAGE (the orchestrator files it). Report: run, frames, route steps with frames, end-save
digests, levels + equipment at the end, upgrades taken / skipped (why), named state, walkthrough corrections (EN-old
room ids, positions), tool gaps.
