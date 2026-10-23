# OpenSummoners — port frontier (what to port next)

> **DERIVED FILE** — `python3 tools/gen_port_ledger.py && python3 tools/gen_frontier.py`.

The unported, non-thunk, engine-proper functions already **called by ported code** — the natural next chips. A **leaf** has all of its
own engine callees ported, so it can land today with zero new dependencies. Sorted by how many ported callers want it.

For the *forward* port path (the title-menu scene runner and what it calls) and the semantic milestone order, see `ROADMAP.md` — some of that path isn't yet reachable from ported code so won't appear here.

- frontier functions: **34**
- of those, zero-dependency **leaves: 21** (recommended order below)

## Leaf shortlist — portable today (top 40 by ported-caller count)

| VA | size | ported callers | band |
|----|-----:|---------------:|------|
| 0x5a4760 | 16 | 2 | launcher config parse + spell fx + bitmap/ZDD render + RNG |
| 0x58c8d0 | 51 | 1 | master sprite-group register + audio/music init + anim pump |
| 0x54e4f0 | 207 | 1 | cutscene dispatcher + sprite copy + tilemap collision + camera |
| 0x4e68d0 | 393 | 1 | narrative scene FSM + dungeon/encounter driver (sparse) |
| 0x523010 | 743 | 1 | narrative scene FSM + dungeon/encounter driver (sparse) |
| 0x52fd80 | 767 | 1 | narrative scene FSM + dungeon/encounter driver (sparse) |
| 0x525ae0 | 899 | 1 | narrative scene FSM + dungeon/encounter driver (sparse) |
| 0x4e1d10 | 1685 | 1 | narrative scene FSM + dungeon/encounter driver (sparse) |
| 0x522310 | 3316 | 1 | narrative scene FSM + dungeon/encounter driver (sparse) |
| 0x4ee690 | 3574 | 1 | narrative scene FSM + dungeon/encounter driver (sparse) |
| 0x535520 | 3936 | 1 | narrative scene FSM + dungeon/encounter driver (sparse) |
| 0x536480 | 4023 | 1 | narrative scene FSM + dungeon/encounter driver (sparse) |
| 0x53ec40 | 5788 | 1 | narrative scene FSM + dungeon/encounter driver (sparse) |
| 0x533580 | 8094 | 1 | narrative scene FSM + dungeon/encounter driver (sparse) |
| 0x4ef4e0 | 9352 | 1 | narrative scene FSM + dungeon/encounter driver (sparse) |
| 0x523300 | 9965 | 1 | narrative scene FSM + dungeon/encounter driver (sparse) |
| 0x537440 | 10805 | 1 | narrative scene FSM + dungeon/encounter driver (sparse) |
| 0x530080 | 13474 | 1 | narrative scene FSM + dungeon/encounter driver (sparse) |
| 0x4f6a10 | 14027 | 1 | narrative scene FSM + dungeon/encounter driver (sparse) |
| 0x5034b0 | 18747 | 1 | narrative scene FSM + dungeon/encounter driver (sparse) |
| 0x510100 | 22636 | 1 | narrative scene FSM + dungeon/encounter driver (sparse) |

## Full frontier by address band

### narrative scene FSM + dungeon/encounter driver (sparse) (29)

| VA | size | ported callers | unported deps | leaf |
|----|-----:|---------------:|--------------:|:----:|
| 0x4e68d0 | 393 | 1 | 0 | ✓ |
| 0x523010 | 743 | 1 | 0 | ✓ |
| 0x52fd80 | 767 | 1 | 0 | ✓ |
| 0x525ae0 | 899 | 1 | 0 | ✓ |
| 0x4e1d10 | 1685 | 1 | 0 | ✓ |
| 0x522310 | 3316 | 1 | 0 | ✓ |
| 0x4ee690 | 3574 | 1 | 0 | ✓ |
| 0x535520 | 3936 | 1 | 0 | ✓ |
| 0x536480 | 4023 | 1 | 0 | ✓ |
| 0x53ec40 | 5788 | 1 | 0 | ✓ |
| 0x533580 | 8094 | 1 | 0 | ✓ |
| 0x4ef4e0 | 9352 | 1 | 0 | ✓ |
| 0x523300 | 9965 | 1 | 0 | ✓ |
| 0x537440 | 10805 | 1 | 0 | ✓ |
| 0x530080 | 13474 | 1 | 0 | ✓ |
| 0x4e23d0 | 13606 | 1 | 1 |  |
| 0x4f6a10 | 14027 | 1 | 0 | ✓ |
| 0x4e6a60 | 15415 | 1 | 2 |  |
| 0x4ea7b0 | 15468 | 1 | 1 |  |
| 0x4fec40 | 17841 | 1 | 1 |  |
| 0x515a00 | 18142 | 1 | 1 |  |
| 0x4fa2a0 | 18188 | 1 | 1 |  |
| 0x5034b0 | 18747 | 1 | 0 | ✓ |
| 0x539e80 | 19641 | 1 | 5 |  |
| 0x4f19e0 | 20437 | 1 | 3 |  |
| 0x510100 | 22636 | 1 | 0 | ✓ |
| 0x51a1d0 | 32877 | 1 | 3 |  |
| 0x507e70 | 32899 | 1 | 2 |  |
| 0x525e70 | 40515 | 1 | 3 |  |

### cutscene dispatcher + sprite copy + tilemap collision + camera (3)

| VA | size | ported callers | unported deps | leaf |
|----|-----:|---------------:|--------------:|:----:|
| 0x54e4f0 | 207 | 1 | 0 | ✓ |
| 0x546670 | 22187 | 1 | 4 |  |
| 0x5402e0 | 25033 | 1 | 1 |  |

### master sprite-group register + audio/music init + anim pump (1)

| VA | size | ported callers | unported deps | leaf |
|----|-----:|---------------:|--------------:|:----:|
| 0x58c8d0 | 51 | 1 | 0 | ✓ |

### launcher config parse + spell fx + bitmap/ZDD render + RNG (1)

| VA | size | ported callers | unported deps | leaf |
|----|-----:|---------------:|--------------:|:----:|
| 0x5a4760 | 16 | 2 | 0 | ✓ |

