---
name: story-after-water-shrine
start: end:rh-water-shrine
steps:
  - loaded into a room with control: world != 0 and control
  - left the start room: room != 0 and frame > 3000
forbid:
  - back at the title after loading: world == 0 and tcount == 5 and frame > 6000
---
**Frontier probe: continue the main story from where the owner's recorded play ends** (scenario `rh-water-shrine`,
the last of the save chain rh-early-game → -2 → -3 → rh-after-prologue → rh-water-shrine). Nothing past that save is
recorded: every new room, scene, enemy and menu you reach is new coverage for the port.

1. `rd new <run> --from end:rh-water-shrine` boots with that play's statue save; at the title pick **Continue** and load
   the newest slot. Find out where you are in the route: `rd look`, `rd text`, the F1 Main Menu → `Messages` (the day's
   hint), and `docs/game/walkthrough.md` (match the area / party / day).
2. Follow the walkthrough's 🔑 steps from there, as far as you can. Save at every goddess statue you pass (drink → save
   prompt) and run `rd endsave <run>` after each save, so later probes can start there; note each digest.
3. Fights: read `docs/game/bosses.md` before a boss; use `--hold` searches (DRIVER.md); keep HP up (F2 quick-heal,
   statues). A game over is fine to see once (it is coverage) — then `rd undo` / `ck load`.
4. Name the state you need as you go (`rd ram R mark` → act → `rd ram R diff` → `rd ram R define`): at least the
   **day** counter, **HP / MP of the leader**, **gold**, and the **party** members. Report each name with its address /
   expression and how you validated it.

Do not grind; do not read other runs' movies. Stop when the run passes ~150k frames, or at a point you cannot get past
after real attempts (say what blocks you). Report: run, frames, the route steps reached with their frames, the
end-save digests, named state, walkthrough corrections (what the guide got wrong for EN-old), tool gaps.
