---
name: story-stella-barness
start: story-stella-road
steps:
  - loaded into a room with control: world != 0 and control
  - left the start room: room != 0 and frame > 3000
forbid:
  - back at the title after loading: world == 0 and tcount == 5 and frame > 6000
---
**Frontier probe: continue the Stella interlude from the end of `story-stella-road`** (start: that scenario's movie
replayed — Day 6, **Stella solo** (Lv3, HP 107/107, MP ~3/53) in **260165**, a stone room with a hanging metal weight —
saved at the White Rose Inn (slot 8, digest `2f229790ba58`)). Read `docs/findings/probes/story-stella-road.md` and the
walkthrough's [RD story-stella-*] notes first. **260165 is the optional Barness-road weight cave** (walkthrough Day 9
💎: needs 3 party members — a dead end for solo Stella; do not spend effort there): go back out 260164 → 260160. **The way
to Barness is 260160's top-left exit** (zone 2, x 0, y 12800), behind a wall the previous driver could not jump: look
at the room (`rd look`, the ledges / platforms / vines, both sides of the room, the rooms it connects to) before
searching; Stella may need a different approach, a run-jump, or a vine to burn (Fire Wall hits ~±17000 lx on BOTH sides
of her, not her own spot). Restore MP at the inn if you need magic. Then: Barness (village 270xxx) → the man by the cave
left of the Inn → **Barness Hills Dungeon, Stella route** (shortcut, light 3 torches, burn vines, moving blocks, clover
lever, the talking doors → Wind Crest) → control returns to Arche → **BOSS: Stella** (`docs/game/bosses.md`).

1. `rd new <run> --from story-stella-road` replays that movie into a live session. Confirm where you are: `rd look`,
   `rd text`, `rd status`.
2. Follow the walkthrough's 🔑 steps from there, as far as you can. Save at every goddess statue / inn save you pass and
   run `rd endsave <run>` after each save, so later probes can start there; note each digest.
3. Fights: read `docs/game/bosses.md` before a boss; use `--hold` searches (DRIVER.md); keep HP up. A game over is fine
   to see once (coverage) — then `rd undo` / `ck load`.
4. Run `rd check` and `rd verify` before your budget runs low (the last driver ran out before verifying).
5. Name the state you need as you go (`rd ram R mark` → act → `rd ram R diff` → `rd ram R define`): anything a
   predicate needs (torch / vine object flags, the boss's HP).

Do not grind; do not read other runs' movies. Stop when the run is ~40k frames past its start (the start is at ~121k), or at a point you cannot get past
after real attempts (say what blocks you). Report: run, frames, the route steps reached with their frames, the
end-save digests, named state, walkthrough corrections (what the guide got wrong for EN-old), tool gaps.
