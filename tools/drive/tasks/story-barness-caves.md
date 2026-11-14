---
name: story-barness-caves
start: end:story-barness-hills
steps:
  - loaded into a room with control: world != 0 and control
  - left the start room: room != 0 and frame > 3000
forbid:
  - back at the title after loading: world == 0 and tcount == 5 and frame > 6000
---
**Frontier probe: continue the main story from where the last driven leg ends** (scenario `story-barness-hills`: Day 6,
Arche solo; its newest SAVE is the Barness inn 270200, slot 7 — from there: inn street 270120, ↑ under the vine arch at lx ≈ 95000 → Barness Hills field 280110). Read `docs/findings/probes/story-barness-hills.md` first: the last
driver could not open the field's cave arches with ↑ — its guess: room exits lock while enemies are engaged (HUD ESCAPE icon
crossed out). Clear / outrun the nearby enemies, then ↑ at the arches; read the room's door / event code in the decompile
(rooms 280110 / 290xxx, the area loader) if pictures do not settle it. Nothing past that save is
recorded: every new room, scene, enemy and menu you reach is new coverage for the port.

1. `rd new <run> --from end:story-barness-hills` boots with that play's statue save; at the title pick **Continue** and load
   the newest slot. Find out where you are in the route: `rd look`, `rd text`, the F1 Main Menu → `Messages` (the day's
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
