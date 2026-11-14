---
name: story-barness-dungeon
start: story-barness-caves
steps:
  - loaded into a room with control: world != 0 and control
  - left the start room: room != 0 and frame > 3000
forbid:
  - back at the title after loading: world == 0 and tcount == 5 and frame > 6000
---
**Frontier probe: continue the main story from the end of `story-barness-caves`** (start: that scenario's whole movie
replayed — Day 6, Arche solo, standing in **280150**, the "end cave" crest-block puzzle, one block already pushed against
the other). Read `docs/findings/probes/story-barness-caves.md` and DRIVER.md (exit zones; push blocks with the sword
SHEATHED) first. Next: solve the block puzzle (its own guess: leave and re-enter 280150 to reset the blocks, then push the
RIGHT block first toward the water past x 176000; walkthrough Day 6 step 7), reach 280155 / the pedestal room (280170?) and
the **Barness Hills Dungeon** (area 290), then follow the walkthrough. Read the room's gimmick code in the decompile when
pictures do not settle a puzzle. Every new room, scene, enemy and menu you reach is new coverage for the port.

1. `rd new <run> --from story-barness-caves` replays that movie into a live session (you are in the game, no title). Confirm where you are: `rd look`, `rd text`, the F1 Main Menu → `Messages` (the day's
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
