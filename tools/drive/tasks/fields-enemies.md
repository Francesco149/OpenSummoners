---
name: fields-enemies
start: end:rh-early-game-3
steps:
  - loaded into a room with control: world != 0 and control
  - fought for a while: frame > 8000
forbid:
  - back at the title after loading: world == 0 and tcount == 5 and frame > 6000
---
**Exercise probe: every enemy type reachable from the `rh-early-game-3` end save** (enemy AI, attacks, drops — retail
behaviour code the port must reproduce). Continue → newest slot; find the day / area first (`rd look`, F1 → Messages,
`docs/game/walkthrough.md`). Read `docs/findings/probes/combat-party.md` (enemy slots `world+0x1160[i]`, code at +0x1d4,
HP `s32(s32(c+0x750)+0x54)`, faction +0x1dc) and DRIVER.md (exit zones) first.

1. Visit the fields / caves open at this point (town ↑ at the tower right of the boutique → 225110, 225120, …; the
   Mountain Cave if reachable). For EACH new enemy type: its code (+0x1d4), max HP, what it does (pictures of its attack),
   the damage it deals to Arche, how many hits it takes, what it drops (`rd text`, `gold`, items).
2. Let each type hit you at least once (guard ↑ once too) and kill it at least once. Keep HP up (F2 / inn); a game over is
   fine once.
3. Note respawn behaviour (leave and re-enter a room).

Do not grind beyond seeing each type. Do not read other runs' movies. Stop around 25k frames. Report: run, frames, a table
enemy / code / room / HP / damage dealt / drop / frame, game facts [RD] (bestiary corrections), tool gaps.
