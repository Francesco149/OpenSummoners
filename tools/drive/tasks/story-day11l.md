---
name: story-day11l
start: save:e50b69b68a6f
steps:
  - in control: world != 0 and control
  - left the fountain room: room != 450225 and room != 0
forbid:
  - back at the title: world == 0 and tcount == 5 and frame > 2000
---
**Frontier probe: Labyrinth of Night 7.5 loot (450200's block-ride chests: [Stronggirl Juice], [Recall Lamp],
[Magician's Outfit]) → 7.6 (very top left pit → [Pitch Black Key]; [Celebrity Ring] (Stella); push the right gargoyle
left → fancy doors; 💎 [White Iron Ore] skeleton)**, from story-d11k's end save (`rd new story-d11l --from
save:e50b69b68a6f` — slot 13, **fountain 2** = 450225, full HP/MP). Party: **Arche Lv17** (Org Slayer, Iron Armor,
Leather Cape, Kitty Ears; Unison + The Full Moon), **Sana Lv17** (Mirror Image), **Stella Lv17**; 20209 CP (cap 120000);
**healing is LOW**: Better Herb ×1, Best Herb ~4, Revival Drink ×3, Revival Drink EX, Magic Candy ×3, Mind Chocolate,
Recall Lamp (+ Org Sword: Help Out). Fountains heal + save for free — use them. **Read
`/mnt/ephemeral/opensummoners/drive/agents/story-d11k/report.md` first**, then the walkthrough's Day 11 step 7 (+ the
[RD story-d11i / d11j / d11k] notes: the labyrinth's EN-old room chain, Unison, the boss).

**Route:**
1. 7.5 loot (worth it: Magician's Outfit = armor; Recall Lamp): 450225 → 450220 → 450200; "Ride stone block right →
   [Stronggirl Juice]; right → [Recall Lamp]; another block, drop, platforms → [Magician's Outfit]" — chests at (232000,
   25600), (321600, 32000), (475200, 44800). Read the Magician's Outfit equip preview (whoever it suits). Back to fountain 2:
   heal + save (`rd endsave`; all 24 slots full: overwrite the shortest play time, keep slots 1–13 and 24).
   ⚠ 450220's top chest (145600, 25600) = teleport trap: don't open it.
2. 7.6: "(Bottom left stairs down.) Very top left, drop pit → [Pitch Black Key]; exit top right; drop, right; top right
   past up-stairs → boost → [Celebrity Ring] (Stella). Bottom right: push right gargoyle left → fancy doors appear.
   💎 first: far left of this area, boost up, examine skeleton → [White Iron Ore] (Help Out)." The gargoyles 70410 at
   (192000 / 220800, 117600) in 450220 are this pair (Arche pushes; followers don't).
3. Stop when the fancy doors have appeared (save at fountain 2 first), at ~40k frames past the start, or at a point you
   cannot get past after real attempts (say what blocks you, with an `rd map` / `rd look` PNG path). **End on a save.**

**Fights:** party HP `s32(s32(s32(world+0x2784)+0x4030+4*i)+0xa64)` for i = 0..2 — heal BEFORE a fight search; avoid
`phmin < 40` (a tighter threshold collapses searches to 1 cell); hostile HP per character slot c:
`s32(s32(c+0x750)+0x54) if u8(s32(c+0x40)) and s32(c+0x1dc) == 2 else 0`; enemy position `s32(s32(c+0x40)+4)`; dormant
enemies `c+0x1d0` == 0; slot numbers move per room; add `room != R` to every in-room search's avoid (a door press
"wins" a fight search). Leader swap `ent` + `F` = quick-heal.

**Tools:** `rd zones R --all` (x = LEFT edge); `rd map R --crop` (no 70202 planks); `rd ck R save/load/drop` — the pool
(~30) can wedge after `ck load` (stale names): `rd stop` clears it; `ck load` / `undo`, NOT `rd rewind`; AVOID
`rd strip` / `--at`. `rd search --hold`. `toward()` holds can turn into a dash (pits!). Unison: sword drawn, hold X ~105
frames, release (10 MP), lasts ~2000 frames, 2 boosts from the air, start on flat ground. Menu message boxes (Life Dew,
herbs) close with X; until then field input looks frozen. Chests / levers / locks with V facing them. Never step scripts
through a Yes/No / save prompt. Searches `-j 1`. Do NOT allow `f2` in searches. Never wrap rd in `timeout`.
`RD_HOST=bunker` for every rd command; one at a time.

Do not grind; do not read other runs' movies. Run `rd check` + `rd verify` ONCE (mandatory; at the end is fine). Your
report goes in your FINAL MESSAGE (the orchestrator files it). Report: run, frames, route steps with frames, end-save
digests, levels + equipment at the end, upgrades taken / skipped (why), named state, walkthrough corrections (EN-old
room ids, positions), tool gaps.
