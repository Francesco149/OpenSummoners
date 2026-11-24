---
name: story-day10d
start: story-d10c
steps:
  - in control in the fountain room: world != 0 and control
  - left 410210: room != 410210 and room != 0
forbid:
  - back at the title: world == 0 and tcount == 5 and frame > 2000
---
**Frontier probe: Weathervane Tower, walkthrough Day 10 steps 2.6 (optional chests) → 2.7 (star wall, levers ×3, fall,
stairs up) → BOSS Wyvern, from the end of run `story-d10c`** (`rd new story-d10d --from story-d10c` — the RUN, its movie
replayed: f417707 in **410210**, the fountain room's lower corridor, control, leader Arche, sword sheathed; HP / MP full
from the fountain). Party: **Arche** Lv13 262/262 (Org Slayer, Scale Armor, Leather Cape, Kitty Ears), **Sana** Lv13
191/191 (Rune Staff, Scale Armor, Chest Sash, Green Smock, Anti-Poison Ring; new ability Antidote), **Stella** Lv12 218/218
(Lady's Dagger, Scale Armor, Woven Gloves, Salamander Scale); 2598 CP; Revival Drink ×2, Water of Life (Help Out),
Recall Lamp, Magic Candy ×3, many Better Herbs. MoH 14 taken. Last save: the fountain, slot 1, `4e700cca7925` (all 24
slots full: a new save overwrites one — pick the oldest, keep slots 1 and 24). **Read
`/mnt/ephemeral/opensummoners/drive/agents/story-d10c/report.md` first** and the walkthrough's **Day 10** ([RD story-d10a]
… [RD story-d10c]). Sana's water attack = forward + X together (quirk #184).

**Route (walkthrough 2.6 tail → 2.7):**
1. Optional chests off 410210, cheapest first; skip one after real attempts if it costs > ~6k frames: "Door → [Mystery
   Juice]; left door → past spikes [Best Herb]; right door, drop off edge → **[Blessed Earring]**". The Blessed Earring
   guards against the harpies' song (confusion) — worth taking; equip it on the member most hurt by confusion (Arche,
   who leads the boss fight) if an accessory slot allows, else say why not. Mystery Juice → Sana or Stella (max MP).
   `rd zones 410210 --all` lists the doors.
2. 🔑 "Left past the open **star wall** (the star lever opened it), stairs, lever ×3 going up → dead end → fall off
   the edge → stairs up (no monsters) → top → **BOSS: Wyvern**". Save at a fountain / statue before the boss if one is
   on the way (`rd endsave`).
3. **BOSS Wyvern**: flame attacks, butt slam, a wind gust that knocks you off the edges; aim for the HEAD (Stella's
   in-game advice), stay in the middle. Watch every member's HP; heal (Better Herbs; Sana's heal) before anyone drops.
   Fight with checkpoints (`rd ck R save`) before the fight and at good moments; use `rd search` with an HP-preserving
   `--score` when hand-play loses too much. Record the boss's max HP (EN-old Easy) via named state if you can find it.

Stop after the Wyvern dies and control returns (the next task takes 2.8: MoH 15, Wind Crest 3), or at ~40k frames past
the start, or at a point you cannot get past after real attempts (say what blocks you, with an `rd map` PNG path).

**Tools:** `rd zones R --all`; `rd map R --crop`; `rd ck R save/load/drop` (the pool runs out after ~20 checkpoints:
drop old ones). Doors in this tower fire on ↑ (the ↑ x can differ from the zone x). Class-1 one-way slopes: jump up
through, `D`+`C` drops through. Chests open only when facing them. Hold `--goal` searches with `--hold N` when the goal is
true mid-jump. Check every search's `found … N frames` line before keeping a path. Searches `-j 1`; every `--score`
search needs the current room in its `--avoid`. Never `rd tap ent` through dialogue that may end in a Yes/No or save
prompt; do NOT allow `f2` in searches. `rd text` does not show menu lists — read menus with `rd look`.

Fights: harpies grab and drop you a floor; harpy song confuses. Do not grind; do not read other runs' movies. **Run
`rd check` and `rd verify` when you reach ~$6 spent / about two thirds of your effort (mandatory)**; verify again
before you finish if you can. Report: run, frames, route steps with frames, end-save digests, each character's level +
equipment at the end, upgrades taken / skipped (why), boss facts, named state, walkthrough corrections, tool gaps.
