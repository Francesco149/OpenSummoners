# OpenSummoners — port frontier (what to port next)

> **DERIVED FILE** — `python3 tools/gen_port_ledger.py && python3 tools/gen_frontier.py`.

The unported, non-thunk, engine-proper functions already **called by ported code** — the natural next chips. A **leaf** has all of its
own engine callees ported, so it can land today with zero new dependencies. Sorted by how many ported callers want it.

For the *forward* port path (the title-menu scene runner and what it calls) and the semantic milestone order, see `ROADMAP.md` — some of that path isn't yet reachable from ported code so won't appear here.

- frontier functions: **311**
- of those, zero-dependency **leaves: 133** (recommended order below)

## Leaf shortlist — portable today (top 40 by ported-caller count)

| VA | size | ported callers | band |
|----|-----:|---------------:|------|
| 0x5bb870 | 16 | 47 | launcher config parse + spell fx + bitmap/ZDD render + RNG |
| 0x5bb8c0 | 31 | 42 | launcher config parse + spell fx + bitmap/ZDD render + RNG |
| 0x5bb890 | 40 | 40 | launcher config parse + spell fx + bitmap/ZDD render + RNG |
| 0x5bb880 | 16 | 36 | launcher config parse + spell fx + bitmap/ZDD render + RNG |
| 0x5bb8e0 | 9 | 16 | launcher config parse + spell fx + bitmap/ZDD render + RNG |
| 0x5bb3a0 | 39 | 4 | launcher config parse + spell fx + bitmap/ZDD render + RNG |
| 0x5bbc60 | 45 | 4 | launcher config parse + spell fx + bitmap/ZDD render + RNG |
| 0x5bbc90 | 55 | 4 | launcher config parse + spell fx + bitmap/ZDD render + RNG |
| 0x5bbc20 | 57 | 4 | launcher config parse + spell fx + bitmap/ZDD render + RNG |
| 0x556e50 | 88 | 4 | cutscene dispatcher + sprite copy + tilemap collision + camera |
| 0x41bbe0 | 312 | 4 | menu/dialog controller + char init + shop/NPC + save path |
| 0x40bb70 | 30 | 3 | object-pool ctor + game-loop FSM + font/glyph + msg fmt |
| 0x5ba650 | 78 | 3 | launcher config parse + spell fx + bitmap/ZDD render + RNG |
| 0x5ba4c0 | 84 | 3 | launcher config parse + spell fx + bitmap/ZDD render + RNG |
| 0x5ba2d0 | 95 | 3 | launcher config parse + spell fx + bitmap/ZDD render + RNG |
| 0x407f30 | 453 | 3 | object-pool ctor + game-loop FSM + font/glyph + msg fmt |
| 0x4c5af0 | 770 | 3 | scene-event dispatch (narrative scripting) + party/inventory |
| 0x5a4760 | 16 | 2 | launcher config parse + spell fx + bitmap/ZDD render + RNG |
| 0x5ba3a0 | 16 | 2 | launcher config parse + spell fx + bitmap/ZDD render + RNG |
| 0x5b6ec0 | 21 | 2 | launcher config parse + spell fx + bitmap/ZDD render + RNG |
| 0x5ba3d0 | 22 | 2 | launcher config parse + spell fx + bitmap/ZDD render + RNG |
| 0x5aff00 | 26 | 2 | launcher config parse + spell fx + bitmap/ZDD render + RNG |
| 0x5bcfb0 | 49 | 2 | launcher config parse + spell fx + bitmap/ZDD render + RNG |
| 0x494e10 | 66 | 2 | tile/sprite grid render + spell fx + battle UI + palette |
| 0x41bb80 | 95 | 2 | menu/dialog controller + char init + shop/NPC + save path |
| 0x579860 | 131 | 2 | master sprite-group register + audio/music init + anim pump |
| 0x579680 | 191 | 2 | master sprite-group register + audio/music init + anim pump |
| 0x495e40 | 411 | 2 | tile/sprite grid render + spell fx + battle UI + palette |
| 0x4051d0 | 3244 | 2 | object-pool ctor + game-loop FSM + font/glyph + msg fmt |
| 0x4034f0 | 7330 | 2 | object-pool ctor + game-loop FSM + font/glyph + msg fmt |
| 0x5ba3f0 | 17 | 1 | launcher config parse + spell fx + bitmap/ZDD render + RNG |
| 0x5ba520 | 21 | 1 | launcher config parse + spell fx + bitmap/ZDD render + RNG |
| 0x5b6580 | 28 | 1 | launcher config parse + spell fx + bitmap/ZDD render + RNG |
| 0x5b0260 | 30 | 1 | launcher config parse + spell fx + bitmap/ZDD render + RNG |
| 0x5b5a90 | 33 | 1 | launcher config parse + spell fx + bitmap/ZDD render + RNG |
| 0x5b64f0 | 38 | 1 | launcher config parse + spell fx + bitmap/ZDD render + RNG |
| 0x4428f0 | 41 | 1 | entity per-frame FSM + action handlers + dialog + skills |
| 0x496170 | 42 | 1 | tile/sprite grid render + spell fx + battle UI + palette |
| 0x5b5a60 | 42 | 1 | launcher config parse + spell fx + bitmap/ZDD render + RNG |
| 0x565810 | 45 | 1 | title + gameplay scene runners + engine init + options + input init |

## Full frontier by address band

### object-pool ctor + game-loop FSM + font/glyph + msg fmt (11)

| VA | size | ported callers | unported deps | leaf |
|----|-----:|---------------:|--------------:|:----:|
| 0x40fe00 | 596 | 10 | 5 |  |
| 0x40a5d0 | 568 | 7 | 5 |  |
| 0x40ac90 | 834 | 4 | 4 |  |
| 0x40bb70 | 30 | 3 | 0 | ✓ |
| 0x407f30 | 453 | 3 | 0 | ✓ |
| 0x409150 | 881 | 2 | 1 |  |
| 0x4051d0 | 3244 | 2 | 0 | ✓ |
| 0x4034f0 | 7330 | 2 | 0 | ✓ |
| 0x40a340 | 498 | 1 | 6 |  |
| 0x409520 | 1093 | 1 | 0 | ✓ |
| 0x408730 | 1114 | 1 | 8 |  |

### menu/dialog controller + char init + shop/NPC + save path (9)

| VA | size | ported callers | unported deps | leaf |
|----|-----:|---------------:|--------------:|:----:|
| 0x41bbe0 | 312 | 4 | 0 | ✓ |
| 0x41bb80 | 95 | 2 | 0 | ✓ |
| 0x415ae0 | 786 | 2 | 1 |  |
| 0x41d780 | 942 | 2 | 4 |  |
| 0x41a890 | 3030 | 2 | 6 |  |
| 0x41cfd0 | 438 | 1 | 0 | ✓ |
| 0x418ec0 | 522 | 1 | 0 | ✓ |
| 0x41b670 | 1295 | 1 | 5 |  |
| 0x4192f0 | 1516 | 1 | 5 |  |

### scene/level init + entity spawn + def-by-id lookup (13)

| VA | size | ported callers | unported deps | leaf |
|----|-----:|---------------:|--------------:|:----:|
| 0x42f840 | 936 | 1 | 0 | ✓ |
| 0x42c130 | 988 | 1 | 0 | ✓ |
| 0x428c00 | 1115 | 1 | 0 | ✓ |
| 0x42c510 | 1119 | 1 | 0 | ✓ |
| 0x42a500 | 1182 | 1 | 0 | ✓ |
| 0x42ff00 | 1267 | 1 | 0 | ✓ |
| 0x42d3c0 | 1628 | 1 | 0 | ✓ |
| 0x42cce0 | 1760 | 1 | 0 | ✓ |
| 0x429db0 | 1861 | 1 | 0 | ✓ |
| 0x42b9a0 | 1929 | 1 | 0 | ✓ |
| 0x42da50 | 2105 | 1 | 0 | ✓ |
| 0x42a9a0 | 2978 | 1 | 5 |  |
| 0x429060 | 3218 | 1 | 1 |  |

### battle scenario init + turn engine + input poll + save mgr (6)

| VA | size | ported callers | unported deps | leaf |
|----|-----:|---------------:|--------------:|:----:|
| 0x43d4b0 | 144 | 1 | 0 | ✓ |
| 0x43c1b0 | 302 | 1 | 2 |  |
| 0x43dd30 | 728 | 1 | 0 | ✓ |
| 0x43d540 | 748 | 1 | 1 |  |
| 0x43d830 | 1254 | 1 | 0 | ✓ |
| 0x430400 | 5039 | 1 | 0 | ✓ |

### entity per-frame FSM + action handlers + dialog + skills (18)

| VA | size | ported callers | unported deps | leaf |
|----|-----:|---------------:|--------------:|:----:|
| 0x4428f0 | 41 | 1 | 0 | ✓ |
| 0x445e50 | 246 | 1 | 0 | ✓ |
| 0x441de0 | 274 | 1 | 0 | ✓ |
| 0x4466b0 | 276 | 1 | 0 | ✓ |
| 0x44bf60 | 751 | 1 | 4 |  |
| 0x44bc10 | 819 | 1 | 4 |  |
| 0x44a200 | 1338 | 1 | 4 |  |
| 0x44d2e0 | 1528 | 1 | 4 |  |
| 0x4484b0 | 1894 | 1 | 0 | ✓ |
| 0x44d8f0 | 2164 | 1 | 3 |  |
| 0x448dc0 | 2221 | 1 | 7 |  |
| 0x449810 | 2221 | 1 | 1 |  |
| 0x44b160 | 2523 | 1 | 4 |  |
| 0x44a740 | 2552 | 1 | 4 |  |
| 0x44c250 | 2972 | 1 | 5 |  |
| 0x44e8d0 | 3428 | 1 | 4 |  |
| 0x44f690 | 3586 | 1 | 9 |  |
| 0x446900 | 3869 | 1 | 4 |  |

### master dialogue runner + action/frame dispatch + sprite batch (13)

| VA | size | ported callers | unported deps | leaf |
|----|-----:|---------------:|--------------:|:----:|
| 0x468b10 | 7214 | 2 | 6 |  |
| 0x468970 | 172 | 1 | 1 |  |
| 0x467260 | 208 | 1 | 0 | ✓ |
| 0x4675c0 | 387 | 1 | 0 | ✓ |
| 0x467750 | 406 | 1 | 0 | ✓ |
| 0x4512d0 | 1012 | 1 | 4 |  |
| 0x450810 | 1485 | 1 | 0 | ✓ |
| 0x452020 | 1649 | 1 | 5 |  |
| 0x4516e0 | 2336 | 1 | 4 |  |
| 0x455d80 | 3175 | 1 | 5 |  |
| 0x452730 | 4119 | 1 | 4 |  |
| 0x456d30 | 4581 | 1 | 4 |  |
| 0x4539b0 | 8699 | 1 | 9 |  |

### battle phase controller + NPC AI + particle fx + damage UI (4)

| VA | size | ported callers | unported deps | leaf |
|----|-----:|---------------:|--------------:|:----:|
| 0x47a300 | 278 | 1 | 0 | ✓ |
| 0x47a7f0 | 376 | 1 | 0 | ✓ |
| 0x47a420 | 685 | 1 | 0 | ✓ |
| 0x479f90 | 875 | 1 | 0 | ✓ |

### char anim + hit-test/knockback + GDI glyph + sfx trigger (7)

| VA | size | ported callers | unported deps | leaf |
|----|-----:|---------------:|--------------:|:----:|
| 0x48c150 | 1374 | 3 | 14 |  |
| 0x48e100 | 246 | 1 | 0 | ✓ |
| 0x48c6b0 | 359 | 1 | 0 | ✓ |
| 0x48d710 | 550 | 1 | 0 | ✓ |
| 0x48cb90 | 1005 | 1 | 0 | ✓ |
| 0x48da70 | 1672 | 1 | 0 | ✓ |
| 0x48ef40 | 4389 | 1 | 6 |  |

### tile/sprite grid render + spell fx + battle UI + palette (22)

| VA | size | ported callers | unported deps | leaf |
|----|-----:|---------------:|--------------:|:----:|
| 0x49bf90 | 424 | 3 | 2 |  |
| 0x494e10 | 66 | 2 | 0 | ✓ |
| 0x495e40 | 411 | 2 | 0 | ✓ |
| 0x496170 | 42 | 1 | 0 | ✓ |
| 0x4961a0 | 63 | 1 | 0 | ✓ |
| 0x49a6e0 | 82 | 1 | 0 | ✓ |
| 0x4961e0 | 95 | 1 | 0 | ✓ |
| 0x496240 | 95 | 1 | 0 | ✓ |
| 0x495dc0 | 119 | 1 | 0 | ✓ |
| 0x499070 | 142 | 1 | 0 | ✓ |
| 0x498820 | 319 | 1 | 0 | ✓ |
| 0x495fe0 | 394 | 1 | 0 | ✓ |
| 0x4963a0 | 446 | 1 | 1 |  |
| 0x491820 | 696 | 1 | 0 | ✓ |
| 0x497c20 | 788 | 1 | 1 |  |
| 0x496560 | 1028 | 1 | 1 |  |
| 0x4975e0 | 1054 | 1 | 1 |  |
| 0x498960 | 1449 | 1 | 0 | ✓ |
| 0x497f40 | 1750 | 1 | 2 |  |
| 0x496ec0 | 1756 | 1 | 2 |  |
| 0x49d930 | 2296 | 1 | 2 |  |
| 0x49f7c0 | 5270 | 1 | 4 |  |

### scene-event dispatch (narrative scripting) + party/inventory (30)

| VA | size | ported callers | unported deps | leaf |
|----|-----:|---------------:|--------------:|:----:|
| 0x4c5af0 | 770 | 3 | 0 | ✓ |
| 0x4b3b20 | 330 | 1 | 0 | ✓ |
| 0x4bd6b0 | 359 | 1 | 1 |  |
| 0x4c1a30 | 491 | 1 | 1 |  |
| 0x4bdf90 | 580 | 1 | 2 |  |
| 0x4b3c70 | 697 | 1 | 3 |  |
| 0x4a63d0 | 706 | 1 | 0 | ✓ |
| 0x4ae150 | 712 | 1 | 4 |  |
| 0x4bdbe0 | 937 | 1 | 4 |  |
| 0x4bd820 | 960 | 1 | 3 |  |
| 0x4bd2e0 | 968 | 1 | 6 |  |
| 0x4c29c0 | 1020 | 1 | 2 |  |
| 0x4a5fa0 | 1057 | 1 | 5 |  |
| 0x4a66a0 | 1244 | 1 | 2 |  |
| 0x4c2310 | 1701 | 1 | 6 |  |
| 0x4c3ee0 | 1727 | 1 | 2 |  |
| 0x4c1c20 | 1774 | 1 | 6 |  |
| 0x4ccc90 | 2610 | 1 | 4 |  |
| 0x4c45a0 | 3423 | 1 | 9 |  |
| 0x4c2dc0 | 4360 | 1 | 2 |  |
| 0x4bbef0 | 5004 | 1 | 1 |  |
| 0x4ae420 | 6478 | 1 | 4 |  |
| 0x4ac6d0 | 6756 | 1 | 2 |  |
| 0x4be220 | 14286 | 1 | 5 |  |
| 0x4cd6d0 | 14951 | 1 | 20 |  |
| 0x4afdb0 | 15577 | 1 | 9 |  |
| 0x4a0c90 | 20785 | 1 | 9 |  |
| 0x4c6b40 | 21496 | 1 | 22 |  |
| 0x4a6b80 | 23051 | 1 | 5 |  |
| 0x4b4d40 | 28488 | 1 | 15 |  |

### narrative scene FSM + dungeon/encounter driver (sparse) (36)

| VA | size | ported callers | unported deps | leaf |
|----|-----:|---------------:|--------------:|:----:|
| 0x4e68d0 | 393 | 1 | 2 |  |
| 0x523010 | 743 | 1 | 2 |  |
| 0x52fd80 | 767 | 1 | 0 | ✓ |
| 0x4e19e0 | 811 | 1 | 6 |  |
| 0x4e61d0 | 817 | 1 | 1 |  |
| 0x4e6520 | 817 | 1 | 1 |  |
| 0x525ae0 | 899 | 1 | 1 |  |
| 0x4e1d10 | 1685 | 1 | 9 |  |
| 0x522310 | 3316 | 1 | 8 |  |
| 0x4ee690 | 3574 | 1 | 10 |  |
| 0x535520 | 3936 | 1 | 9 |  |
| 0x536480 | 4023 | 1 | 13 |  |
| 0x53ec40 | 5788 | 1 | 9 |  |
| 0x4d64f0 | 6202 | 1 | 13 |  |
| 0x533580 | 8094 | 1 | 9 |  |
| 0x4ef4e0 | 9352 | 1 | 10 |  |
| 0x523300 | 9965 | 1 | 11 |  |
| 0x537440 | 10805 | 1 | 11 |  |
| 0x530080 | 13474 | 1 | 12 |  |
| 0x4e23d0 | 13606 | 1 | 13 |  |
| 0x4f6a10 | 14027 | 1 | 18 |  |
| 0x4e6a60 | 15415 | 1 | 15 |  |
| 0x4ea7b0 | 15468 | 1 | 20 |  |
| 0x4fec40 | 17841 | 1 | 11 |  |
| 0x515a00 | 18142 | 1 | 12 |  |
| 0x4d7d80 | 18166 | 1 | 10 |  |
| 0x4fa2a0 | 18188 | 1 | 23 |  |
| 0x5034b0 | 18747 | 1 | 17 |  |
| 0x539e80 | 19641 | 1 | 18 |  |
| 0x4d13a0 | 20018 | 1 | 17 |  |
| 0x4f19e0 | 20437 | 1 | 21 |  |
| 0x4dc510 | 21461 | 1 | 23 |  |
| 0x510100 | 22636 | 1 | 8 |  |
| 0x51a1d0 | 32877 | 1 | 18 |  |
| 0x507e70 | 32899 | 1 | 15 |  |
| 0x525e70 | 40515 | 1 | 13 |  |

### cutscene dispatcher + sprite copy + tilemap collision + camera (15)

| VA | size | ported callers | unported deps | leaf |
|----|-----:|---------------:|--------------:|:----:|
| 0x556e50 | 88 | 4 | 0 | ✓ |
| 0x55fdb0 | 461 | 2 | 3 |  |
| 0x5534d0 | 76 | 1 | 0 | ✓ |
| 0x5556d0 | 179 | 1 | 0 | ✓ |
| 0x54e4f0 | 207 | 1 | 0 | ✓ |
| 0x552f60 | 590 | 1 | 4 |  |
| 0x54ed80 | 592 | 1 | 4 |  |
| 0x552c70 | 748 | 1 | 4 |  |
| 0x553ec0 | 1681 | 1 | 4 |  |
| 0x554570 | 1724 | 1 | 6 |  |
| 0x54f140 | 2094 | 1 | 2 |  |
| 0x554c40 | 2187 | 1 | 5 |  |
| 0x553520 | 2458 | 1 | 4 |  |
| 0x546670 | 22187 | 1 | 28 |  |
| 0x5402e0 | 25033 | 1 | 14 |  |

### title + gameplay scene runners + engine init + options + input init (33)

| VA | size | ported callers | unported deps | leaf |
|----|-----:|---------------:|--------------:|:----:|
| 0x562af0 | 82 | 3 | 1 |  |
| 0x566250 | 169 | 3 | 1 |  |
| 0x56d710 | 510 | 2 | 2 |  |
| 0x565810 | 45 | 1 | 0 | ✓ |
| 0x564690 | 47 | 1 | 0 | ✓ |
| 0x56ded0 | 61 | 1 | 2 |  |
| 0x56c430 | 64 | 1 | 0 | ✓ |
| 0x56de80 | 67 | 1 | 1 |  |
| 0x564110 | 80 | 1 | 0 | ✓ |
| 0x566300 | 80 | 1 | 0 | ✓ |
| 0x5640b0 | 81 | 1 | 1 |  |
| 0x56bfd0 | 88 | 1 | 0 | ✓ |
| 0x56da40 | 96 | 1 | 0 | ✓ |
| 0x56cb90 | 113 | 1 | 0 | ✓ |
| 0x565840 | 115 | 1 | 0 | ✓ |
| 0x5646c0 | 180 | 1 | 3 |  |
| 0x56d620 | 237 | 1 | 3 |  |
| 0x56dd80 | 254 | 1 | 0 | ✓ |
| 0x56cc10 | 267 | 1 | 0 | ✓ |
| 0x56daa0 | 267 | 1 | 0 | ✓ |
| 0x56dfe0 | 417 | 1 | 0 | ✓ |
| 0x5642e0 | 440 | 1 | 7 |  |
| 0x5644a0 | 483 | 1 | 5 |  |
| 0x565b00 | 528 | 1 | 0 | ✓ |
| 0x566350 | 540 | 1 | 1 |  |
| 0x568780 | 554 | 1 | 2 |  |
| 0x5658c0 | 565 | 1 | 9 |  |
| 0x568b40 | 659 | 1 | 0 | ✓ |
| 0x56c640 | 690 | 1 | 2 |  |
| 0x568de0 | 998 | 1 | 3 |  |
| 0x56a670 | 1011 | 1 | 10 |  |
| 0x561d20 | 1259 | 1 | 3 |  |
| 0x5624c0 | 1355 | 1 | 9 |  |

### master sprite-group register + audio/music init + anim pump (8)

| VA | size | ported callers | unported deps | leaf |
|----|-----:|---------------:|--------------:|:----:|
| 0x579860 | 131 | 2 | 0 | ✓ |
| 0x579680 | 191 | 2 | 0 | ✓ |
| 0x579740 | 285 | 2 | 2 |  |
| 0x58c8d0 | 51 | 1 | 0 | ✓ |
| 0x583ee0 | 248 | 1 | 0 | ✓ |
| 0x5798f0 | 259 | 1 | 0 | ✓ |
| 0x583c90 | 580 | 1 | 0 | ✓ |
| 0x583fe0 | 979 | 1 | 12 |  |

### inventory/menu + audio cue mgr + render dispatch + scene load (10)

| VA | size | ported callers | unported deps | leaf |
|----|-----:|---------------:|--------------:|:----:|
| 0x59d540 | 334 | 1 | 4 |  |
| 0x590480 | 465 | 1 | 3 |  |
| 0x590f20 | 498 | 1 | 0 | ✓ |
| 0x59ec30 | 531 | 1 | 3 |  |
| 0x590ca0 | 592 | 1 | 3 |  |
| 0x59ad60 | 615 | 1 | 5 |  |
| 0x59d160 | 644 | 1 | 1 |  |
| 0x59e230 | 924 | 1 | 12 |  |
| 0x59e5d0 | 1033 | 1 | 8 |  |
| 0x591120 | 1096 | 1 | 8 |  |

### launcher config parse + spell fx + bitmap/ZDD render + RNG (76)

| VA | size | ported callers | unported deps | leaf |
|----|-----:|---------------:|--------------:|:----:|
| 0x5bb870 | 16 | 47 | 0 | ✓ |
| 0x5bb8c0 | 31 | 42 | 0 | ✓ |
| 0x5bb890 | 40 | 40 | 0 | ✓ |
| 0x5bb880 | 16 | 36 | 0 | ✓ |
| 0x5bb8e0 | 9 | 16 | 0 | ✓ |
| 0x5bb2f0 | 140 | 5 | 2 |  |
| 0x5bb250 | 150 | 5 | 2 |  |
| 0x5bb3a0 | 39 | 4 | 0 | ✓ |
| 0x5bbc60 | 45 | 4 | 0 | ✓ |
| 0x5bbc90 | 55 | 4 | 0 | ✓ |
| 0x5bbc20 | 57 | 4 | 0 | ✓ |
| 0x5bbcd0 | 72 | 4 | 1 |  |
| 0x5bcb80 | 611 | 4 | 3 |  |
| 0x5b64b0 | 47 | 3 | 2 |  |
| 0x5b6060 | 53 | 3 | 1 |  |
| 0x5ba650 | 78 | 3 | 0 | ✓ |
| 0x5ba4c0 | 84 | 3 | 0 | ✓ |
| 0x5ba2d0 | 95 | 3 | 0 | ✓ |
| 0x5bc0c0 | 143 | 3 | 1 |  |
| 0x5a4760 | 16 | 2 | 0 | ✓ |
| 0x5ba3a0 | 16 | 2 | 0 | ✓ |
| 0x5b6ec0 | 21 | 2 | 0 | ✓ |
| 0x5ba3d0 | 22 | 2 | 0 | ✓ |
| 0x5aff00 | 26 | 2 | 0 | ✓ |
| 0x5bcfb0 | 49 | 2 | 0 | ✓ |
| 0x5bcb30 | 70 | 2 | 1 |  |
| 0x5ba120 | 154 | 2 | 1 |  |
| 0x5a3670 | 499 | 2 | 1 |  |
| 0x5ba3f0 | 17 | 1 | 0 | ✓ |
| 0x5ba520 | 21 | 1 | 0 | ✓ |
| 0x5b6580 | 28 | 1 | 0 | ✓ |
| 0x5b0260 | 30 | 1 | 0 | ✓ |
| 0x5b5a90 | 33 | 1 | 0 | ✓ |
| 0x5b64f0 | 38 | 1 | 0 | ✓ |
| 0x5b5a60 | 42 | 1 | 0 | ✓ |
| 0x5b5d50 | 52 | 1 | 0 | ✓ |
| 0x5b9450 | 52 | 1 | 0 | ✓ |
| 0x5bbb10 | 56 | 1 | 0 | ✓ |
| 0x5b0390 | 66 | 1 | 0 | ✓ |
| 0x5b68f0 | 67 | 1 | 2 |  |
| 0x5b61b0 | 68 | 1 | 1 |  |
| 0x5b6200 | 78 | 1 | 1 |  |
| 0x5b0d70 | 82 | 1 | 0 | ✓ |
| 0x5b5ef0 | 87 | 1 | 0 | ✓ |
| 0x5afb90 | 96 | 1 | 0 | ✓ |
| 0x5bbdb0 | 102 | 1 | 2 |  |
| 0x5b03e0 | 112 | 1 | 0 | ✓ |
| 0x5b01e0 | 124 | 1 | 0 | ✓ |
| 0x5b65b0 | 127 | 1 | 2 |  |
| 0x5b0c60 | 128 | 1 | 0 | ✓ |
| 0x5afb00 | 129 | 1 | 0 | ✓ |
| 0x5b6db0 | 129 | 1 | 1 |  |
| 0x5b0ce0 | 131 | 1 | 0 | ✓ |
| 0x5bbe20 | 135 | 1 | 1 |  |
| 0x5afbf0 | 146 | 1 | 0 | ✓ |
| 0x5b9cf0 | 159 | 1 | 3 |  |
| 0x5b9fc0 | 164 | 1 | 3 |  |
| 0x5aff20 | 186 | 1 | 0 | ✓ |
| 0x5baed0 | 193 | 1 | 3 |  |
| 0x5b60e0 | 202 | 1 | 1 |  |
| 0x5b0280 | 264 | 1 | 0 | ✓ |
| 0x5b0dd0 | 267 | 1 | 0 | ✓ |
| 0x5b0ee0 | 331 | 1 | 0 | ✓ |
| 0x5affe0 | 409 | 1 | 0 | ✓ |
| 0x5b6bc0 | 491 | 1 | 8 |  |
| 0x5b10d0 | 520 | 1 | 3 |  |
| 0x5bbeb0 | 527 | 1 | 1 |  |
| 0x5b7970 | 539 | 1 | 0 | ✓ |
| 0x5b5af0 | 595 | 1 | 0 | ✓ |
| 0x5afc90 | 623 | 1 | 8 |  |
| 0x5a3870 | 828 | 1 | 0 | ✓ |
| 0x5bc7c0 | 880 | 1 | 3 |  |
| 0x5b0890 | 961 | 1 | 0 | ✓ |
| 0x5b3b00 | 1002 | 1 | 1 |  |
| 0x5b0450 | 1075 | 1 | 0 | ✓ |
| 0x5b1f10 | 2345 | 1 | 3 |  |

