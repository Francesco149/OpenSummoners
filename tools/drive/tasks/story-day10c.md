---
name: story-day10c
start: story-d10b
steps:
  - in control in the tower: world != 0 and control
  - left 410215: room != 410215 and room != 0
forbid:
  - back at the title: world == 0 and tcount == 5 and frame > 2000
---
**Frontier probe: Weathervane Tower, walkthrough Day 10 step 2.6 (the wheeled-tower puzzle → star lever → Stronggirl
Juice → fountain → MoH 14), from the end of run `story-d10b`** (its movie replayed: f407920 in **410215**, control,
leader Arche, sword drawn; the tower 70480 back at its start, b0x 51600). Party: **Arche** Lv13 262/262 (Org Slayer,
Scale Armor, Leather Cape, Kitty Ears), **Sana** ~Lv13 116 HP (**Rune Staff**, Scale Armor, Chest Sash, Green Smock,
Anti-Poison Ring), **Stella** ~Lv12–13 ~98 HP (unchanged); 2598 CP; Revival Drink ×2, Water of Life (Help Out). Last
save: White Rose Inn, slot 24, `eaee1a401f88` (no save in d10b). **Read
`/mnt/ephemeral/opensummoners/drive/agents/story-d10b/report.md` first** (its "Blocker" section is this task) and the
walkthrough's **Day 10** ([RD story-d10a], [RD story-d10b]). Sana's water attack = forward + X together (quirk #184).

**The blocker (d10b):** in 410215 the star lever 70216 (12800, 19200) sits on the left ledge (x 0–38400, top 25600);
the wheeled tower 70480 (b0x 51600, ~9000 wide, top platform 100144 at y 38400) stands on the floor (y 64000); slope
steps at (126400, 46400 / 56000) by the right door; the left door zone 2 (19200, 60800) → **410210** is behind the
tower. From the right Arche can only push the tower LEFT (stops at b0x 38400); she cannot pass it, and jumps reach ly
≈ 40000 (the top needs ≈ 32800). Walkthrough: "Arche pushes platform right; Sana / Stella stays on it; push back →
they reach ledge → star lever" ⇒ someone rides the top while Arche pushes, then jumps tower top → ledge. **Only Arche
pushes** (quirk-level fact from 260200). Leaving + re-entering 410215 resets the tower.

Try, cheapest first, and measure each with `rd ram` / named state (b0x, character-slot x / y via `actors.sh`):
1. **Followers pass objects?** Push the tower left to the ledge, then watch Sana's / Stella's slot x — do they end up
   on the tower's LEFT side or ON its top (follower AI jumps / warps to the leader)? A follower on top = park it with
   Follow Mode **"Stay There!"** (the command menu; one member at a time) and push.
2. **Steps route:** push the tower RIGHT is impossible from the right, but a follower parked on the step (126400,
   46400) can jump to the top if the tower stands beside the steps — check whether pushing left then the tower's own
   motion / a re-entry leaves it anywhere useful.
3. **From 410210:** the intended entry may be the LEFT door (behind the tower). 410210 is reached from 410190's lower
   area, zone 3 (211200, 144000) — find the way into that area (the "hidden stairs down" may be a different spot than
   the chamber d10b used: probe other drops off 410190's right edge / ledges; `rd zones 410190 --all`).
4. RE fallback (say so in the report, don't guess): the push code for 70480 and what 100144 is (rider / lift).

Then: down, stairs up [Stronggirl Juice]; drop → **fountain** (save: `rd endsave`); right → **MoH 14** (upgrade per
DRIVER.md "keep the party strong"). Stop there (or at ~40k frames).

**Tools:** `rd zones R --all`; `rd map R --crop`; `rd ck R save/load/drop` (pool runs out after ~20 checkpoints:
drop old ones). `rd zones` "walk in" labels are unreliable here (these doors fire on ↑; the ↑ x can differ from the
zone x). Check every search's `found … N frames` line before keeping a path (an 8.8k-frame path got applied in d10b).
Searches `-j 1`; every `--score` search needs the current room in its `--avoid`. Never `rd tap ent` through dialogue
that may end in a Yes/No or save prompt; do NOT allow `f2` in searches.

Fights: harpies grab and drop you a floor; harpy song confuses; watch each member's HP. Do not grind; do not read other
runs' movies. **Run `rd check` and `rd verify` when you reach ~$6 spent (mandatory)**; verify again before `finish()`
if the budget allows. Stop at ~40k frames past the start, or at a point you cannot get past after real attempts (say
what blocks you, with an `rd map` PNG path). Report: run, frames, route steps with frames, end-save digests, each
character's level + equipment at the end, upgrades taken / skipped (why), named state, walkthrough corrections, tool gaps.
