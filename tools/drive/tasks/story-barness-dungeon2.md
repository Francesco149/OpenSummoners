---
name: story-barness-dungeon2
start: story-barness-dungeon
steps:
  - loaded into a room with control: world != 0 and control
  - left the start room: room != 0 and frame > 3000
forbid:
  - back at the title after loading: world == 0 and tcount == 5 and frame > 6000
---
**Frontier probe: continue the main story from the end of `story-barness-dungeon`** (start: that scenario's movie
replayed — Day 6, Arche solo in **290130** of the Barness Hills Dungeon, HP ~40/172, NO healing items, a Cocorat (slot
`fh3`, ~76 HP) near the ↑ stairs door to 290150 at x 150400 on the bottom floor). Read
`docs/findings/probes/story-barness-dungeon.md` (route, checkpoints, named state) and DRIVER.md (exit zones: kill the
Cocorat first; blocks) first. Low HP + no herbs: fight with `--hold` searches and `--avoid "hp < 15"`; a game over returns
to 270120 with full HP (shortcut gates 280115501 / 280115502 are open) — acceptable if you cannot win the fight.
Next (walkthrough Day 6 / guide 8.5): 290150 (moving block / weight), the gargoyle question (answer 12), then the Stella
interlude. Read room gimmick code in the decompile when pictures do not settle a puzzle. Every new room, scene, enemy and
menu you reach is new coverage for the port.

1. `rd new <run> --from story-barness-dungeon` replays that movie into a live session (you are in the game, no title). Confirm where you are: `rd look`, `rd text`, the F1 Main Menu → `Messages` (the day's
   hint), and `docs/game/walkthrough.md` (match the area / party / day).
2. Follow the walkthrough's 🔑 steps from there, as far as you can. Save at every goddess statue you pass (drink → save
   prompt) and run `rd endsave <run>` after each save, so later probes can start there; note each digest.
3. Fights: read `docs/game/bosses.md` before a boss; use `--hold` searches (DRIVER.md); keep HP up (F2 quick-heal,
   statues). A game over is fine to see once (it is coverage) — then `rd undo` / `ck load`.
4. Name the state you need as you go (`rd ram R mark` → act → `rd ram R diff` → `rd ram R define`): at least the
   **day** counter, **HP / MP of the leader**, **gold**, and the **party** members. Report each name with its address /
   expression and how you validated it.

Do not grind; do not read other runs' movies. Stop when the run passes ~100k frames, or at a point you cannot get past
after real attempts (say what blocks you). Report: run, frames, the route steps reached with their frames, the
end-save digests, named state, walkthrough corrections (what the guide got wrong for EN-old), tool gaps.
