---
name: story-stella-interlude
start: story-barness-dungeon3
steps:
  - loaded into a room with control: world != 0 and control
  - left the start room: room != 0 and frame > 3000
forbid:
  - back at the title after loading: world == 0 and tcount == 5 and frame > 6000
---
**Frontier probe: continue the Stella interlude from the end of `story-barness-dungeon3`** (start: that scenario's movie
replayed — Day 6, **Stella solo** (`party` = 4, party slot 2, HP 81/81, MP 51/51) on the UPPER floor of Mayberk Manor
**310120**, just back from the hall 310130; the portrait scene in 310140 and the Rowen scene are done). Read
`docs/findings/probes/story-barness-dungeon3.md` and the walkthrough's [RD story-barness-dungeon3] note (the Chartreux
room map) first. The previous driver could not find the way down from the manor's upper floor to the garden 310110: look
before searching (`rd look`, `rd text`, try every ↑ / ↓ at stairs and doors on that floor; the walkthrough lists the
manor rooms: father's room, downstairs room, kitchen, 1F left double doors = Rowen). Then: out of the manor, through
Chartreux to the EAST (the city gate refused before Rowen; it should open now), the road toward Barness (chest under a
ledge, lake street), the **White Rose Inn** (save there), Stella burns the vines, Barness → the man by the cave left of
the Inn → **Barness Hills Dungeon, Stella route** (torches, vines, Wind Crest), then control returns to Arche → **BOSS:
Stella** (`docs/game/bosses.md`). Every new room, scene, enemy, spell (Stella = fire magic: learn her spell select and
cast) and menu is new coverage for the port.

1. `rd new <run> --from story-barness-dungeon3` replays that movie into a live session. Confirm where you are: `rd look`,
   `rd text`, `rd status`.
2. Follow the walkthrough's 🔑 steps from there, as far as you can. Save at every goddess statue / inn save you pass and
   run `rd endsave <run>` after each save, so later probes can start there; note each digest.
3. Fights: read `docs/game/bosses.md` before a boss; use `--hold` searches (DRIVER.md); keep HP up. A game over is fine
   to see once (coverage) — then `rd undo` / `ck load`.
4. Name the state you need as you go (`rd ram R mark` → act → `rd ram R diff` → `rd ram R define`): Stella's spell
   select / cast state, the leader id for Stella (`lid` reads 0 for her), anything a predicate needs.

Do not grind; do not read other runs' movies. Stop when the run passes ~100k frames, or at a point you cannot get past
after real attempts (say what blocks you). Report: run, frames, the route steps reached with their frames, the
end-save digests, named state, walkthrough corrections (what the guide got wrong for EN-old), tool gaps.
