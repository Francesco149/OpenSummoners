---
name: story-barness-dungeon3
start: story-barness-dungeon2
steps:
  - loaded into a room with control: world != 0 and control
  - left the start room: room != 0 and frame > 3000
forbid:
  - back at the title after loading: world == 0 and tcount == 5 and frame > 6000
---
**Frontier probe: continue the main story from the end of `story-barness-dungeon2`** (start: that scenario's movie
replayed — Day 6, Arche solo in **290150** below the weight shaft (lx ~215000, ly 80800), HP ~161/172, Better Herbs in the
bag; the 290152 lever already pulled, so the weight moves). Read `docs/findings/probes/story-barness-dungeon2.md` and the
walkthrough's [RD story-barness-dungeon2] note first. Next: kill the Kobold + Killer Bat near the shaft (they hold the exit
zone), ride the weight up, zone 2 of 290150 (walk-in x 252800, y 44800–60800) → 290155, the gargoyle question (answer 12),
then the Stella interlude (Stella solo). Save at any goddess statue (`rd endsave`). Every new room, scene, enemy and menu is
new coverage for the port.

1. `rd new <run> --from story-barness-dungeon2` replays that movie into a live session (you are in the game, no title). Confirm where you are: `rd look`, `rd text`, the F1 Main Menu → `Messages` (the day's
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
