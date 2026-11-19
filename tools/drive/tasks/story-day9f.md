---
name: story-day9f
start: story-d9e
steps:
  - in control in the Catacombs: world != 0 and control
  - got past the gate room: room != 400180 and room != 0 and room != 400160
forbid:
  - back at the title: world == 0 and tcount == 5 and frame > 2000
---
**Frontier probe: Chartreux Catacombs, the rest of walkthrough Day 9 step 4.6 (the second Crypt Key) → 4.7 BOSS Master
Ghost → 4.8 MoH 13 → the guard → [Thieves' Key] → save, from the end of run `story-d9e`** (its movie replayed: f293977
in **400180**, the right half of the room past the x 149400 gate, control, party 7). Party **Lv11**: **Arche** 144/225
HP, **Sana** 109/158 HP ~79 MP, **Stella 52/195 HP ~32 MP** (heal first); gear as in story-d9d (Stella: Lady's Dagger,
Scale Armor, Woven Gloves, Salamander Scale); 8810 CP; Magic Candy ×2, Ghost Marshmallow ×3 (200–260 HP each), Better
Herb, Revival Drink, Venom-Away S ×3, Recall Lamp, Doll Hand, Memory Photo 6; **no Crypt Key**. Last save: White Rose
Inn, slot 22, `a2a16253c9bb` (none since). Read `/mnt/ephemeral/opensummoners/drive/agents/story-d9e/report.md` and the
walkthrough's **Day 9** step 4 ([RD story-d9c] … [RD story-d9e]) first.

**Tools:** `rd zones R --all` (doors, objects + "zone hold"); object state `s32(s32(world+0x11e0+4*slot)+0xc)`; `rd map R
--crop` (world units: walls, pits, objects); `rd ck R save/load/drop` (load + undo are fast; `rd rewind` replays from
boot — prefer checkpoints). Keep `{…}` inputs short (598-token limit). Every `--score` search needs the current room in
its `--avoid`; bound `lx`. Never mash `ent` through Yes/No or save prompts; do NOT allow `f2` in searches. Leader swap
`ent`: check `lid` after each press.

Next:
1. 🔑 Find the second Crypt Key (walkthrough 4.6: "the right door needs another key → **left door** (fall into skeletons)
   → left pit hugging RIGHT → [Crypt Key], [Guardian Drop]; bottom [Power Drop]"). The x 335000 gate (→ 400190 / 400185)
   needs it. Map 400180 (and 400160) first: look for a door / zone not yet used — leads: the floor spike platform 70430
   at (238000, 24600), the upper room with a 70800 at (246400, 12800), any pit on the left. `rd map` + `rd zones --all`.
2. 🔑 4.7: the fountain (right of the fancy doors) → the doors → **BOSS: Master Ghost** → [Pocketwatch]. Full HP/MP
   first. Ghosts often ignore the sword: Stella's Phoenix (slot 3).
3. 🔑 4.8: stairs down: left [Crypt Key] + **MoH 13**; key right, stairs → the "moaning" guard → give [Pocketwatch] →
   [Thieves' Key] → **save** (`rd endsave`). Stop there (or at ~40k frames).
Save at every save point and `rd endsave <run>` after each; note each digest.

Fights: `--hold` searches; keep an avoid on each member's HP. Do not grind; do not read other runs' movies. **Run `rd
check` and `rd verify` when you reach ~$6 spent (mandatory)**; verify again before `finish()` if the budget allows. Stop
when the run is ~40k frames past its start, or at a point you cannot get past after real attempts (say what blocks you,
with an `rd map` PNG path). Report: run, frames, route steps with frames, end-save digests, each character's level +
equipment at the end, upgrades taken / skipped (why), shop lists + prices seen, named state, walkthrough corrections,
tool gaps.
