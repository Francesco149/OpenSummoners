# OpenSummoners — port frontier (what to port next)

> **DERIVED FILE** — `python3 tools/gen_port_ledger.py && python3 tools/gen_frontier.py`.

The unported, non-thunk, engine-proper functions already **called by ported code** — the natural next chips. A **leaf** has all of its
own engine callees ported, so it can land today with zero new dependencies. Sorted by how many ported callers want it.

For the *forward* port path (the title-menu scene runner and what it calls) and the semantic milestone order, see `ROADMAP.md` — some of that path isn't yet reachable from ported code so won't appear here.

- frontier functions: **245**
- of those, zero-dependency **leaves: 143** (recommended order below)

## Leaf shortlist — portable today (top 40 by ported-caller count)

| VA | size | ported callers | band |
|----|-----:|---------------:|------|
| 0x41bbe0 | 312 | 4 | menu/dialog controller + char init + shop/NPC + save path |
| 0x5bbc60 | 45 | 3 | launcher config parse + spell fx + bitmap/ZDD render + RNG |
| 0x5bbc90 | 55 | 3 | launcher config parse + spell fx + bitmap/ZDD render + RNG |
| 0x412db0 | 57 | 3 | menu/dialog controller + char init + shop/NPC + save path |
| 0x5bbc20 | 57 | 3 | launcher config parse + spell fx + bitmap/ZDD render + RNG |
| 0x417870 | 106 | 3 | menu/dialog controller + char init + shop/NPC + save path |
| 0x412d30 | 117 | 3 | menu/dialog controller + char init + shop/NPC + save path |
| 0x4124d0 | 146 | 3 | menu/dialog controller + char init + shop/NPC + save path |
| 0x4182d0 | 408 | 3 | menu/dialog controller + char init + shop/NPC + save path |
| 0x49a050 | 13 | 2 | tile/sprite grid render + spell fx + battle UI + palette |
| 0x5a4760 | 16 | 2 | launcher config parse + spell fx + bitmap/ZDD render + RNG |
| 0x5ba3a0 | 16 | 2 | launcher config parse + spell fx + bitmap/ZDD render + RNG |
| 0x5b6ec0 | 21 | 2 | launcher config parse + spell fx + bitmap/ZDD render + RNG |
| 0x5ba3d0 | 22 | 2 | launcher config parse + spell fx + bitmap/ZDD render + RNG |
| 0x491740 | 38 | 2 | tile/sprite grid render + spell fx + battle UI + palette |
| 0x4c57f0 | 50 | 2 | scene-event dispatch (narrative scripting) + party/inventory |
| 0x409090 | 67 | 2 | object-pool ctor + game-loop FSM + font/glyph + msg fmt |
| 0x411ec0 | 118 | 2 | menu/dialog controller + char init + shop/NPC + save path |
| 0x562a70 | 127 | 2 | title + gameplay scene runners + engine init + options + input init |
| 0x4118b0 | 134 | 2 | menu/dialog controller + char init + shop/NPC + save path |
| 0x43c920 | 143 | 2 | battle scenario init + turn engine + input poll + save mgr |
| 0x56df10 | 203 | 2 | title + gameplay scene runners + engine init + options + input init |
| 0x412c40 | 227 | 2 | menu/dialog controller + char init + shop/NPC + save path |
| 0x562d50 | 239 | 2 | title + gameplay scene runners + engine init + options + input init |
| 0x40f800 | 511 | 2 | object-pool ctor + game-loop FSM + font/glyph + msg fmt |
| 0x4c5830 | 694 | 2 | scene-event dispatch (narrative scripting) + party/inventory |
| 0x4c5af0 | 770 | 2 | scene-event dispatch (narrative scripting) + party/inventory |
| 0x4051d0 | 3244 | 2 | object-pool ctor + game-loop FSM + font/glyph + msg fmt |
| 0x4034f0 | 7330 | 2 | object-pool ctor + game-loop FSM + font/glyph + msg fmt |
| 0x5bb870 | 16 | 1 | launcher config parse + spell fx + bitmap/ZDD render + RNG |
| 0x5ba3f0 | 17 | 1 | launcher config parse + spell fx + bitmap/ZDD render + RNG |
| 0x58d090 | 18 | 1 | master sprite-group register + audio/music init + anim pump |
| 0x5ba520 | 21 | 1 | launcher config parse + spell fx + bitmap/ZDD render + RNG |
| 0x40f5a0 | 25 | 1 | object-pool ctor + game-loop FSM + font/glyph + msg fmt |
| 0x5aff00 | 26 | 1 | launcher config parse + spell fx + bitmap/ZDD render + RNG |
| 0x58cfc0 | 27 | 1 | master sprite-group register + audio/music init + anim pump |
| 0x5a3bf0 | 27 | 1 | launcher config parse + spell fx + bitmap/ZDD render + RNG |
| 0x587d30 | 28 | 1 | master sprite-group register + audio/music init + anim pump |
| 0x5b6580 | 28 | 1 | launcher config parse + spell fx + bitmap/ZDD render + RNG |
| 0x40bb70 | 30 | 1 | object-pool ctor + game-loop FSM + font/glyph + msg fmt |

## Full frontier by address band

### object-pool ctor + game-loop FSM + font/glyph + msg fmt (20)

| VA | size | ported callers | unported deps | leaf |
|----|-----:|---------------:|--------------:|:----:|
| 0x40a5d0 | 568 | 4 | 5 |  |
| 0x40fe00 | 596 | 3 | 5 |  |
| 0x409090 | 67 | 2 | 0 | ✓ |
| 0x40f800 | 511 | 2 | 0 | ✓ |
| 0x4051d0 | 3244 | 2 | 0 | ✓ |
| 0x4034f0 | 7330 | 2 | 0 | ✓ |
| 0x40f5a0 | 25 | 1 | 0 | ✓ |
| 0x40bb70 | 30 | 1 | 0 | ✓ |
| 0x4022a0 | 47 | 1 | 0 | ✓ |
| 0x40fdb0 | 70 | 1 | 0 | ✓ |
| 0x4022d0 | 88 | 1 | 0 | ✓ |
| 0x40e2f0 | 100 | 1 | 0 | ✓ |
| 0x408d40 | 124 | 1 | 0 | ✓ |
| 0x4071f0 | 196 | 1 | 0 | ✓ |
| 0x40df40 | 374 | 1 | 0 | ✓ |
| 0x409e90 | 375 | 1 | 2 |  |
| 0x40b8f0 | 626 | 1 | 3 |  |
| 0x408dc0 | 714 | 1 | 5 |  |
| 0x4017d0 | 1175 | 1 | 1 |  |
| 0x40c380 | 5077 | 1 | 99 |  |

### menu/dialog controller + char init + shop/NPC + save path (26)

| VA | size | ported callers | unported deps | leaf |
|----|-----:|---------------:|--------------:|:----:|
| 0x41bbe0 | 312 | 4 | 0 | ✓ |
| 0x412db0 | 57 | 3 | 0 | ✓ |
| 0x417870 | 106 | 3 | 0 | ✓ |
| 0x412d30 | 117 | 3 | 0 | ✓ |
| 0x4124d0 | 146 | 3 | 0 | ✓ |
| 0x4182d0 | 408 | 3 | 0 | ✓ |
| 0x411390 | 413 | 3 | 5 |  |
| 0x411ec0 | 118 | 2 | 0 | ✓ |
| 0x4118b0 | 134 | 2 | 0 | ✓ |
| 0x412c40 | 227 | 2 | 0 | ✓ |
| 0x41a890 | 3030 | 2 | 6 |  |
| 0x413b20 | 68 | 1 | 0 | ✓ |
| 0x41bb80 | 95 | 1 | 0 | ✓ |
| 0x412330 | 98 | 1 | 0 | ✓ |
| 0x419c00 | 110 | 1 | 0 | ✓ |
| 0x411560 | 117 | 1 | 0 | ✓ |
| 0x41a7f0 | 152 | 1 | 0 | ✓ |
| 0x413760 | 173 | 1 | 0 | ✓ |
| 0x415860 | 198 | 1 | 0 | ✓ |
| 0x41cfd0 | 438 | 1 | 0 | ✓ |
| 0x4134f0 | 613 | 1 | 0 | ✓ |
| 0x411c50 | 623 | 1 | 2 |  |
| 0x41dc90 | 890 | 1 | 2 |  |
| 0x41e600 | 907 | 1 | 5 |  |
| 0x41b670 | 1295 | 1 | 6 |  |
| 0x410610 | 3103 | 1 | 1 |  |

### scene/level init + entity spawn + def-by-id lookup (2)

| VA | size | ported callers | unported deps | leaf |
|----|-----:|---------------:|--------------:|:----:|
| 0x426f70 | 37 | 1 | 0 | ✓ |
| 0x429d00 | 68 | 1 | 0 | ✓ |

### battle scenario init + turn engine + input poll + save mgr (7)

| VA | size | ported callers | unported deps | leaf |
|----|-----:|---------------:|--------------:|:----:|
| 0x43e3a0 | 516 | 3 | 1 |  |
| 0x43c920 | 143 | 2 | 0 | ✓ |
| 0x43e140 | 266 | 2 | 4 |  |
| 0x43c610 | 52 | 1 | 0 | ✓ |
| 0x43c170 | 53 | 1 | 0 | ✓ |
| 0x43c9b0 | 130 | 1 | 0 | ✓ |
| 0x43c650 | 696 | 1 | 1 |  |

### entity per-frame FSM + action handlers + dialog + skills (2)

| VA | size | ported callers | unported deps | leaf |
|----|-----:|---------------:|--------------:|:----:|
| 0x448c80 | 47 | 1 | 0 | ✓ |
| 0x4467d0 | 289 | 1 | 2 |  |

### master dialogue runner + action/frame dispatch + sprite batch (1)

| VA | size | ported callers | unported deps | leaf |
|----|-----:|---------------:|--------------:|:----:|
| 0x468b10 | 7214 | 1 | 6 |  |

### battle phase controller + NPC AI + particle fx + damage UI (2)

| VA | size | ported callers | unported deps | leaf |
|----|-----:|---------------:|--------------:|:----:|
| 0x479d30 | 99 | 1 | 0 | ✓ |
| 0x47b7c0 | 163 | 1 | 0 | ✓ |

### char anim + hit-test/knockback + GDI glyph + sfx trigger (6)

| VA | size | ported callers | unported deps | leaf |
|----|-----:|---------------:|--------------:|:----:|
| 0x48e100 | 246 | 1 | 0 | ✓ |
| 0x48c6b0 | 359 | 1 | 0 | ✓ |
| 0x48d710 | 550 | 1 | 0 | ✓ |
| 0x48cb90 | 1005 | 1 | 0 | ✓ |
| 0x48da70 | 1672 | 1 | 0 | ✓ |
| 0x48ef40 | 4389 | 1 | 8 |  |

### tile/sprite grid render + spell fx + battle UI + palette (34)

| VA | size | ported callers | unported deps | leaf |
|----|-----:|---------------:|--------------:|:----:|
| 0x49a050 | 13 | 2 | 0 | ✓ |
| 0x491740 | 38 | 2 | 0 | ✓ |
| 0x496170 | 42 | 1 | 0 | ✓ |
| 0x4961a0 | 63 | 1 | 0 | ✓ |
| 0x494e10 | 66 | 1 | 0 | ✓ |
| 0x49a2f0 | 76 | 1 | 0 | ✓ |
| 0x4961e0 | 95 | 1 | 0 | ✓ |
| 0x496240 | 95 | 1 | 0 | ✓ |
| 0x497b40 | 98 | 1 | 0 | ✓ |
| 0x497bb0 | 98 | 1 | 0 | ✓ |
| 0x495dc0 | 119 | 1 | 0 | ✓ |
| 0x499070 | 142 | 1 | 0 | ✓ |
| 0x49abd0 | 168 | 1 | 0 | ✓ |
| 0x4962a0 | 242 | 1 | 0 | ✓ |
| 0x49a340 | 291 | 1 | 0 | ✓ |
| 0x498820 | 319 | 1 | 0 | ✓ |
| 0x498f10 | 344 | 1 | 0 | ✓ |
| 0x495fe0 | 394 | 1 | 0 | ✓ |
| 0x495e40 | 411 | 1 | 0 | ✓ |
| 0x49bf90 | 424 | 1 | 2 |  |
| 0x4963a0 | 446 | 1 | 1 |  |
| 0x49a470 | 622 | 1 | 1 |  |
| 0x49a060 | 651 | 1 | 3 |  |
| 0x49ac80 | 664 | 1 | 1 |  |
| 0x491820 | 696 | 1 | 0 | ✓ |
| 0x497c20 | 788 | 1 | 1 |  |
| 0x49c2f0 | 808 | 1 | 6 |  |
| 0x496560 | 1028 | 1 | 3 |  |
| 0x4975e0 | 1054 | 1 | 4 |  |
| 0x4969b0 | 1272 | 1 | 3 |  |
| 0x498960 | 1449 | 1 | 0 | ✓ |
| 0x497f40 | 1750 | 1 | 5 |  |
| 0x496ec0 | 1756 | 1 | 2 |  |
| 0x49af40 | 3313 | 1 | 8 |  |

### scene-event dispatch (narrative scripting) + party/inventory (9)

| VA | size | ported callers | unported deps | leaf |
|----|-----:|---------------:|--------------:|:----:|
| 0x4c57f0 | 50 | 2 | 0 | ✓ |
| 0x4c5830 | 694 | 2 | 0 | ✓ |
| 0x4c5af0 | 770 | 2 | 0 | ✓ |
| 0x4c6830 | 337 | 1 | 2 |  |
| 0x4c63a0 | 391 | 1 | 3 |  |
| 0x4c6990 | 419 | 1 | 0 | ✓ |
| 0x4c6550 | 618 | 1 | 6 |  |
| 0x4cc5a0 | 628 | 1 | 2 |  |
| 0x4c5e00 | 778 | 1 | 3 |  |

### narrative scene FSM + dungeon/encounter driver (sparse) (4)

| VA | size | ported callers | unported deps | leaf |
|----|-----:|---------------:|--------------:|:----:|
| 0x4e61a0 | 33 | 1 | 0 | ✓ |
| 0x4e1950 | 135 | 1 | 0 | ✓ |
| 0x52fd80 | 767 | 1 | 0 | ✓ |
| 0x4e59a0 | 1676 | 1 | 18 |  |

### cutscene dispatcher + sprite copy + tilemap collision + camera (4)

| VA | size | ported callers | unported deps | leaf |
|----|-----:|---------------:|--------------:|:----:|
| 0x55f550 | 199 | 1 | 0 | ✓ |
| 0x54bfb0 | 801 | 1 | 3 |  |
| 0x54c640 | 812 | 1 | 1 |  |
| 0x55f790 | 1558 | 1 | 0 | ✓ |

### title + gameplay scene runners + engine init + options + input init (43)

| VA | size | ported callers | unported deps | leaf |
|----|-----:|---------------:|--------------:|:----:|
| 0x566250 | 169 | 3 | 1 |  |
| 0x562a70 | 127 | 2 | 0 | ✓ |
| 0x56df10 | 203 | 2 | 0 | ✓ |
| 0x562d50 | 239 | 2 | 0 | ✓ |
| 0x56dc20 | 344 | 2 | 1 |  |
| 0x56d710 | 510 | 2 | 3 |  |
| 0x561280 | 37 | 1 | 0 | ✓ |
| 0x565810 | 45 | 1 | 0 | ✓ |
| 0x564690 | 47 | 1 | 0 | ✓ |
| 0x5612b0 | 56 | 1 | 0 | ✓ |
| 0x56ded0 | 61 | 1 | 2 |  |
| 0x56c430 | 64 | 1 | 0 | ✓ |
| 0x56de80 | 67 | 1 | 1 |  |
| 0x564110 | 80 | 1 | 0 | ✓ |
| 0x566300 | 80 | 1 | 0 | ✓ |
| 0x5640b0 | 81 | 1 | 1 |  |
| 0x562af0 | 82 | 1 | 1 |  |
| 0x56bfd0 | 88 | 1 | 0 | ✓ |
| 0x56da40 | 96 | 1 | 0 | ✓ |
| 0x5614e0 | 113 | 1 | 1 |  |
| 0x56cb90 | 113 | 1 | 0 | ✓ |
| 0x565840 | 115 | 1 | 0 | ✓ |
| 0x561150 | 126 | 1 | 0 | ✓ |
| 0x5611d0 | 173 | 1 | 2 |  |
| 0x5646c0 | 180 | 1 | 3 |  |
| 0x56d620 | 237 | 1 | 3 |  |
| 0x56dd80 | 254 | 1 | 0 | ✓ |
| 0x56cc10 | 267 | 1 | 0 | ✓ |
| 0x56daa0 | 267 | 1 | 0 | ✓ |
| 0x56dfe0 | 417 | 1 | 0 | ✓ |
| 0x562ba0 | 432 | 1 | 5 |  |
| 0x5642e0 | 440 | 1 | 7 |  |
| 0x5644a0 | 483 | 1 | 6 |  |
| 0x565b00 | 528 | 1 | 7 |  |
| 0x566350 | 540 | 1 | 3 |  |
| 0x568780 | 554 | 1 | 2 |  |
| 0x5658c0 | 565 | 1 | 10 |  |
| 0x568b40 | 659 | 1 | 7 |  |
| 0x560e90 | 699 | 1 | 0 | ✓ |
| 0x568de0 | 998 | 1 | 14 |  |
| 0x56a670 | 1011 | 1 | 18 |  |
| 0x561d20 | 1259 | 1 | 12 |  |
| 0x5624c0 | 1355 | 1 | 12 |  |

### master sprite-group register + audio/music init + anim pump (26)

| VA | size | ported callers | unported deps | leaf |
|----|-----:|---------------:|--------------:|:----:|
| 0x58d090 | 18 | 1 | 0 | ✓ |
| 0x58cfc0 | 27 | 1 | 0 | ✓ |
| 0x587d30 | 28 | 1 | 0 | ✓ |
| 0x58d060 | 45 | 1 | 0 | ✓ |
| 0x58c8d0 | 51 | 1 | 0 | ✓ |
| 0x58d020 | 51 | 1 | 0 | ✓ |
| 0x58cfe0 | 63 | 1 | 0 | ✓ |
| 0x587ce0 | 71 | 1 | 0 | ✓ |
| 0x587db0 | 76 | 1 | 0 | ✓ |
| 0x58cf60 | 83 | 1 | 0 | ✓ |
| 0x587d50 | 96 | 1 | 0 | ✓ |
| 0x5851e0 | 114 | 1 | 0 | ✓ |
| 0x5878a0 | 207 | 1 | 0 | ✓ |
| 0x58e6a0 | 218 | 1 | 1 |  |
| 0x583ee0 | 248 | 1 | 0 | ✓ |
| 0x58d0b0 | 337 | 1 | 4 |  |
| 0x58d210 | 403 | 1 | 1 |  |
| 0x58e170 | 441 | 1 | 2 |  |
| 0x583c90 | 580 | 1 | 0 | ✓ |
| 0x58e330 | 873 | 1 | 0 | ✓ |
| 0x583fe0 | 979 | 1 | 13 |  |
| 0x58cb30 | 1065 | 1 | 0 | ✓ |
| 0x58e780 | 1823 | 1 | 7 |  |
| 0x58f360 | 3030 | 1 | 43 |  |
| 0x58d460 | 3341 | 1 | 5 |  |
| 0x5752e0 | 17310 | 1 | 9 |  |

### inventory/menu + audio cue mgr + render dispatch + scene load (6)

| VA | size | ported callers | unported deps | leaf |
|----|-----:|---------------:|--------------:|:----:|
| 0x59e1a0 | 34 | 1 | 1 |  |
| 0x59db70 | 227 | 1 | 0 | ✓ |
| 0x59cfc0 | 405 | 1 | 2 |  |
| 0x59ec30 | 531 | 1 | 5 |  |
| 0x59e230 | 924 | 1 | 19 |  |
| 0x59e5d0 | 1033 | 1 | 14 |  |

### launcher config parse + spell fx + bitmap/ZDD render + RNG (53)

| VA | size | ported callers | unported deps | leaf |
|----|-----:|---------------:|--------------:|:----:|
| 0x5bbc60 | 45 | 3 | 0 | ✓ |
| 0x5bbc90 | 55 | 3 | 0 | ✓ |
| 0x5bbc20 | 57 | 3 | 0 | ✓ |
| 0x5bbcd0 | 72 | 3 | 1 |  |
| 0x5bcb80 | 611 | 3 | 3 |  |
| 0x5a4760 | 16 | 2 | 0 | ✓ |
| 0x5ba3a0 | 16 | 2 | 0 | ✓ |
| 0x5b6ec0 | 21 | 2 | 0 | ✓ |
| 0x5ba3d0 | 22 | 2 | 0 | ✓ |
| 0x5b64b0 | 47 | 2 | 2 |  |
| 0x5b6060 | 53 | 2 | 1 |  |
| 0x5bc0c0 | 143 | 2 | 1 |  |
| 0x5ba120 | 154 | 2 | 1 |  |
| 0x5bb870 | 16 | 1 | 0 | ✓ |
| 0x5ba3f0 | 17 | 1 | 0 | ✓ |
| 0x5ba520 | 21 | 1 | 0 | ✓ |
| 0x5aff00 | 26 | 1 | 0 | ✓ |
| 0x5a3bf0 | 27 | 1 | 0 | ✓ |
| 0x5b6580 | 28 | 1 | 0 | ✓ |
| 0x5a4460 | 30 | 1 | 1 |  |
| 0x5bb8c0 | 31 | 1 | 0 | ✓ |
| 0x5a3c10 | 37 | 1 | 0 | ✓ |
| 0x5b64f0 | 38 | 1 | 0 | ✓ |
| 0x5bb3a0 | 39 | 1 | 0 | ✓ |
| 0x5bb890 | 40 | 1 | 0 | ✓ |
| 0x5a3de0 | 49 | 1 | 0 | ✓ |
| 0x5b9450 | 52 | 1 | 0 | ✓ |
| 0x5bbb10 | 56 | 1 | 0 | ✓ |
| 0x5b6300 | 60 | 1 | 0 | ✓ |
| 0x5a3bb0 | 62 | 1 | 0 | ✓ |
| 0x5b68f0 | 67 | 1 | 2 |  |
| 0x5b61b0 | 68 | 1 | 1 |  |
| 0x5bcb30 | 70 | 1 | 1 |  |
| 0x5b6200 | 78 | 1 | 1 |  |
| 0x5ba650 | 78 | 1 | 0 | ✓ |
| 0x5ba4c0 | 84 | 1 | 0 | ✓ |
| 0x5ba2d0 | 95 | 1 | 0 | ✓ |
| 0x5bbdb0 | 102 | 1 | 2 |  |
| 0x5b65b0 | 127 | 1 | 2 |  |
| 0x5b6db0 | 129 | 1 | 1 |  |
| 0x5bb2f0 | 140 | 1 | 2 |  |
| 0x5bb250 | 150 | 1 | 2 |  |
| 0x5b9cf0 | 159 | 1 | 3 |  |
| 0x5b9fc0 | 164 | 1 | 3 |  |
| 0x5baed0 | 193 | 1 | 3 |  |
| 0x5a3c40 | 404 | 1 | 0 | ✓ |
| 0x5a3e20 | 455 | 1 | 0 | ✓ |
| 0x5b6bc0 | 491 | 1 | 8 |  |
| 0x5a3670 | 499 | 1 | 1 |  |
| 0x5a3ff0 | 516 | 1 | 8 |  |
| 0x5b10d0 | 520 | 1 | 3 |  |
| 0x5bbeb0 | 527 | 1 | 1 |  |
| 0x5a3870 | 828 | 1 | 0 | ✓ |

