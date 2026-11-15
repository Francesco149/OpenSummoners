---
name: story-stella-road
start: story-stella-interlude
steps:
  - loaded into a room with control: world != 0 and control
  - left the start room: room != 0 and frame > 3000
forbid:
  - back at the title after loading: world == 0 and tcount == 5 and frame > 6000
---
**Frontier probe: continue the Stella interlude from the end of `story-stella-interlude`** (start: that scenario's
movie replayed — Day 6, **Stella solo** (`party` = 4, slot 2, HP ~59/81, MP ~45/51) in **265120**, the second road room
east of Chartreux, with control). Read `docs/findings/probes/story-stella-interlude.md` and the walkthrough's [RD
story-barness-dungeon3] / [RD story-stella-interlude] notes first (Fire Wall: hold X ~95 frames, release; MP 3). Next, per
the walkthrough's Stella interlude: walk RIGHT (east) toward Barness — chest under a ledge, the lake street ledges, the
**White Rose Inn** (rest + SAVE there: `rd endsave`), Stella burns the vines, Barness → the man by the cave left of the Inn
→ **Barness Hills Dungeon, Stella route** (shortcut, light 3 torches, burn vines, moving blocks, clover lever, the talking
doors → Wind Crest), then control returns to Arche → **BOSS: Stella** (`docs/game/bosses.md`). Every new room, scene,
enemy and menu is new coverage for the port.

1. `rd new <run> --from story-stella-interlude` replays that movie into a live session. Confirm where you are: `rd look`,
   `rd text`, `rd status`.
2. Follow the walkthrough's 🔑 steps from there, as far as you can. Save at every goddess statue / inn save you pass and
   run `rd endsave <run>` after each save, so later probes can start there; note each digest.
3. Fights: read `docs/game/bosses.md` before a boss; use `--hold` searches (DRIVER.md); keep HP up. A game over is fine
   to see once (coverage) — then `rd undo` / `ck load`.
4. Name the state you need as you go (`rd ram R mark` → act → `rd ram R diff` → `rd ram R define`): anything a
   predicate needs (torch / vine object flags, the boss's HP).

Do not grind; do not read other runs' movies. Stop when the run is ~40k frames past its start (the start is at ~99k), or at a point you cannot get past
after real attempts (say what blocks you). Report: run, frames, the route steps reached with their frames, the
end-save digests, named state, walkthrough corrections (what the guide got wrong for EN-old), tool gaps.
