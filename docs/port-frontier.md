# OpenSummoners — port frontier (what to port next)

> **DERIVED FILE** — `python3 tools/gen_port_ledger.py && python3 tools/gen_frontier.py`.

The unported, non-thunk, engine-proper functions already **called by ported code** — the natural next chips. A **leaf** has all of its
own engine callees ported, so it can land today with zero new dependencies. Sorted by how many ported callers want it.

For the *forward* port path (the title-menu scene runner and what it calls) and the semantic milestone order, see `ROADMAP.md` — some of that path isn't yet reachable from ported code so won't appear here.

- frontier functions: **65**
- of those, zero-dependency **leaves: 42** (recommended order below)

## Leaf shortlist — portable today (top 40 by ported-caller count)

| VA | size | ported callers | band |
|----|-----:|---------------:|------|
| 0x556e50 | 88 | 4 | cutscene dispatcher + sprite copy + tilemap collision + camera |
| 0x40ac90 | 834 | 4 | object-pool ctor + game-loop FSM + font/glyph + msg fmt |
| 0x407f30 | 453 | 3 | object-pool ctor + game-loop FSM + font/glyph + msg fmt |
| 0x55fdb0 | 461 | 3 | cutscene dispatcher + sprite copy + tilemap collision + camera |
| 0x4c5af0 | 770 | 3 | scene-event dispatch (narrative scripting) + party/inventory |
| 0x5a4760 | 16 | 2 | launcher config parse + spell fx + bitmap/ZDD render + RNG |
| 0x4cc0e0 | 360 | 2 | scene-event dispatch (narrative scripting) + party/inventory |
| 0x415ae0 | 786 | 2 | menu/dialog controller + char init + shop/NPC + save path |
| 0x4428f0 | 41 | 1 | entity per-frame FSM + action handlers + dialog + skills |
| 0x58c8d0 | 51 | 1 | master sprite-group register + audio/music init + anim pump |
| 0x5534d0 | 76 | 1 | cutscene dispatcher + sprite copy + tilemap collision + camera |
| 0x41db30 | 109 | 1 | menu/dialog controller + char init + shop/NPC + save path |
| 0x43d4b0 | 144 | 1 | battle scenario init + turn engine + input poll + save mgr |
| 0x41dba0 | 148 | 1 | menu/dialog controller + char init + shop/NPC + save path |
| 0x468970 | 172 | 1 | master dialogue runner + action/frame dispatch + sprite batch |
| 0x54e4f0 | 207 | 1 | cutscene dispatcher + sprite copy + tilemap collision + camera |
| 0x441de0 | 274 | 1 | entity per-frame FSM + action handlers + dialog + skills |
| 0x47a300 | 278 | 1 | battle phase controller + NPC AI + particle fx + damage UI |
| 0x43c1b0 | 302 | 1 | battle scenario init + turn engine + input poll + save mgr |
| 0x41e4c0 | 307 | 1 | menu/dialog controller + char init + shop/NPC + save path |
| 0x47a7f0 | 376 | 1 | battle phase controller + NPC AI + particle fx + damage UI |
| 0x4675c0 | 387 | 1 | master dialogue runner + action/frame dispatch + sprite batch |
| 0x4e68d0 | 393 | 1 | narrative scene FSM + dungeon/encounter driver (sparse) |
| 0x467750 | 406 | 1 | master dialogue runner + action/frame dispatch + sprite batch |
| 0x41b470 | 506 | 1 | menu/dialog controller + char init + shop/NPC + save path |
| 0x54ed80 | 592 | 1 | cutscene dispatcher + sprite copy + tilemap collision + camera |
| 0x47a420 | 685 | 1 | battle phase controller + NPC AI + particle fx + damage UI |
| 0x43dd30 | 728 | 1 | battle scenario init + turn engine + input poll + save mgr |
| 0x523010 | 743 | 1 | narrative scene FSM + dungeon/encounter driver (sparse) |
| 0x44bf60 | 751 | 1 | entity per-frame FSM + action handlers + dialog + skills |
| 0x52fd80 | 767 | 1 | narrative scene FSM + dungeon/encounter driver (sparse) |
| 0x525ae0 | 899 | 1 | narrative scene FSM + dungeon/encounter driver (sparse) |
| 0x43d830 | 1254 | 1 | battle scenario init + turn engine + input poll + save mgr |
| 0x522310 | 3316 | 1 | narrative scene FSM + dungeon/encounter driver (sparse) |
| 0x535520 | 3936 | 1 | narrative scene FSM + dungeon/encounter driver (sparse) |
| 0x536480 | 4023 | 1 | narrative scene FSM + dungeon/encounter driver (sparse) |
| 0x533580 | 8094 | 1 | narrative scene FSM + dungeon/encounter driver (sparse) |
| 0x4ef4e0 | 9352 | 1 | narrative scene FSM + dungeon/encounter driver (sparse) |
| 0x523300 | 9965 | 1 | narrative scene FSM + dungeon/encounter driver (sparse) |
| 0x537440 | 10805 | 1 | narrative scene FSM + dungeon/encounter driver (sparse) |

## Full frontier by address band

### object-pool ctor + game-loop FSM + font/glyph + msg fmt (2)

| VA | size | ported callers | unported deps | leaf |
|----|-----:|---------------:|--------------:|:----:|
| 0x40ac90 | 834 | 4 | 0 | ✓ |
| 0x407f30 | 453 | 3 | 0 | ✓ |

### menu/dialog controller + char init + shop/NPC + save path (7)

| VA | size | ported callers | unported deps | leaf |
|----|-----:|---------------:|--------------:|:----:|
| 0x41c390 | 1919 | 10 | 1 |  |
| 0x415ae0 | 786 | 2 | 0 | ✓ |
| 0x41db30 | 109 | 1 | 0 | ✓ |
| 0x41dba0 | 148 | 1 | 0 | ✓ |
| 0x41e4c0 | 307 | 1 | 0 | ✓ |
| 0x41b470 | 506 | 1 | 0 | ✓ |
| 0x4192f0 | 1516 | 1 | 1 |  |

### battle scenario init + turn engine + input poll + save mgr (5)

| VA | size | ported callers | unported deps | leaf |
|----|-----:|---------------:|--------------:|:----:|
| 0x43d4b0 | 144 | 1 | 0 | ✓ |
| 0x43c1b0 | 302 | 1 | 0 | ✓ |
| 0x43dd30 | 728 | 1 | 0 | ✓ |
| 0x43d540 | 748 | 1 | 1 |  |
| 0x43d830 | 1254 | 1 | 0 | ✓ |

### entity per-frame FSM + action handlers + dialog + skills (3)

| VA | size | ported callers | unported deps | leaf |
|----|-----:|---------------:|--------------:|:----:|
| 0x4428f0 | 41 | 1 | 0 | ✓ |
| 0x441de0 | 274 | 1 | 0 | ✓ |
| 0x44bf60 | 751 | 1 | 0 | ✓ |

### master dialogue runner + action/frame dispatch + sprite batch (3)

| VA | size | ported callers | unported deps | leaf |
|----|-----:|---------------:|--------------:|:----:|
| 0x468970 | 172 | 1 | 0 | ✓ |
| 0x4675c0 | 387 | 1 | 0 | ✓ |
| 0x467750 | 406 | 1 | 0 | ✓ |

### battle phase controller + NPC AI + particle fx + damage UI (3)

| VA | size | ported callers | unported deps | leaf |
|----|-----:|---------------:|--------------:|:----:|
| 0x47a300 | 278 | 1 | 0 | ✓ |
| 0x47a7f0 | 376 | 1 | 0 | ✓ |
| 0x47a420 | 685 | 1 | 0 | ✓ |

### tile/sprite grid render + spell fx + battle UI + palette (2)

| VA | size | ported callers | unported deps | leaf |
|----|-----:|---------------:|--------------:|:----:|
| 0x49d330 | 754 | 9 | 1 |  |
| 0x49eea0 | 2332 | 2 | 1 |  |

### scene-event dispatch (narrative scripting) + party/inventory (2)

| VA | size | ported callers | unported deps | leaf |
|----|-----:|---------------:|--------------:|:----:|
| 0x4c5af0 | 770 | 3 | 0 | ✓ |
| 0x4cc0e0 | 360 | 2 | 0 | ✓ |

### narrative scene FSM + dungeon/encounter driver (sparse) (29)

| VA | size | ported callers | unported deps | leaf |
|----|-----:|---------------:|--------------:|:----:|
| 0x4e68d0 | 393 | 1 | 0 | ✓ |
| 0x523010 | 743 | 1 | 0 | ✓ |
| 0x52fd80 | 767 | 1 | 0 | ✓ |
| 0x525ae0 | 899 | 1 | 0 | ✓ |
| 0x4e1d10 | 1685 | 1 | 2 |  |
| 0x522310 | 3316 | 1 | 0 | ✓ |
| 0x4ee690 | 3574 | 1 | 1 |  |
| 0x535520 | 3936 | 1 | 0 | ✓ |
| 0x536480 | 4023 | 1 | 0 | ✓ |
| 0x53ec40 | 5788 | 1 | 1 |  |
| 0x533580 | 8094 | 1 | 0 | ✓ |
| 0x4ef4e0 | 9352 | 1 | 0 | ✓ |
| 0x523300 | 9965 | 1 | 0 | ✓ |
| 0x537440 | 10805 | 1 | 0 | ✓ |
| 0x530080 | 13474 | 1 | 0 | ✓ |
| 0x4e23d0 | 13606 | 1 | 2 |  |
| 0x4f6a10 | 14027 | 1 | 1 |  |
| 0x4e6a60 | 15415 | 1 | 3 |  |
| 0x4ea7b0 | 15468 | 1 | 2 |  |
| 0x4fec40 | 17841 | 1 | 1 |  |
| 0x515a00 | 18142 | 1 | 1 |  |
| 0x4fa2a0 | 18188 | 1 | 2 |  |
| 0x5034b0 | 18747 | 1 | 2 |  |
| 0x539e80 | 19641 | 1 | 6 |  |
| 0x4f19e0 | 20437 | 1 | 5 |  |
| 0x510100 | 22636 | 1 | 0 | ✓ |
| 0x51a1d0 | 32877 | 1 | 5 |  |
| 0x507e70 | 32899 | 1 | 2 |  |
| 0x525e70 | 40515 | 1 | 3 |  |

### cutscene dispatcher + sprite copy + tilemap collision + camera (7)

| VA | size | ported callers | unported deps | leaf |
|----|-----:|---------------:|--------------:|:----:|
| 0x556e50 | 88 | 4 | 0 | ✓ |
| 0x55fdb0 | 461 | 3 | 0 | ✓ |
| 0x5534d0 | 76 | 1 | 0 | ✓ |
| 0x54e4f0 | 207 | 1 | 0 | ✓ |
| 0x54ed80 | 592 | 1 | 0 | ✓ |
| 0x546670 | 22187 | 1 | 6 |  |
| 0x5402e0 | 25033 | 1 | 3 |  |

### master sprite-group register + audio/music init + anim pump (1)

| VA | size | ported callers | unported deps | leaf |
|----|-----:|---------------:|--------------:|:----:|
| 0x58c8d0 | 51 | 1 | 0 | ✓ |

### launcher config parse + spell fx + bitmap/ZDD render + RNG (1)

| VA | size | ported callers | unported deps | leaf |
|----|-----:|---------------:|--------------:|:----:|
| 0x5a4760 | 16 | 2 | 0 | ✓ |

