# OpenSummoners — port frontier (what to port next)

> **DERIVED FILE** — `python3 tools/gen_port_ledger.py && python3 tools/gen_frontier.py`.

The unported, non-thunk, engine-proper functions already **called by ported code** — the natural next chips. A **leaf** has all of its
own engine callees ported, so it can land today with zero new dependencies. Sorted by how many ported callers want it.

For the *forward* port path (the title-menu scene runner and what it calls) and the semantic milestone order, see `ROADMAP.md` — some of that path isn't yet reachable from ported code so won't appear here.

- frontier functions: **395**
- of those, zero-dependency **leaves: 216** (recommended order below)

## Leaf shortlist — portable today (top 40 by ported-caller count)

| VA | size | ported callers | band |
|----|-----:|---------------:|------|
| 0x5bb870 | 16 | 22 | launcher config parse + spell fx + bitmap/ZDD render + RNG |
| 0x5bb8c0 | 31 | 18 | launcher config parse + spell fx + bitmap/ZDD render + RNG |
| 0x5bb890 | 40 | 16 | launcher config parse + spell fx + bitmap/ZDD render + RNG |
| 0x5bb880 | 16 | 15 | launcher config parse + spell fx + bitmap/ZDD render + RNG |
| 0x5bb8e0 | 9 | 10 | launcher config parse + spell fx + bitmap/ZDD render + RNG |
| 0x5bb3a0 | 39 | 4 | launcher config parse + spell fx + bitmap/ZDD render + RNG |
| 0x5bbc60 | 45 | 4 | launcher config parse + spell fx + bitmap/ZDD render + RNG |
| 0x5bbc90 | 55 | 4 | launcher config parse + spell fx + bitmap/ZDD render + RNG |
| 0x5bbc20 | 57 | 4 | launcher config parse + spell fx + bitmap/ZDD render + RNG |
| 0x41bbe0 | 312 | 4 | menu/dialog controller + char init + shop/NPC + save path |
| 0x4c57f0 | 50 | 3 | scene-event dispatch (narrative scripting) + party/inventory |
| 0x409090 | 67 | 3 | object-pool ctor + game-loop FSM + font/glyph + msg fmt |
| 0x413b20 | 68 | 3 | menu/dialog controller + char init + shop/NPC + save path |
| 0x556e50 | 88 | 3 | cutscene dispatcher + sprite copy + tilemap collision + camera |
| 0x412d30 | 117 | 3 | menu/dialog controller + char init + shop/NPC + save path |
| 0x4e1950 | 135 | 3 | narrative scene FSM + dungeon/encounter driver (sparse) |
| 0x4124d0 | 146 | 3 | menu/dialog controller + char init + shop/NPC + save path |
| 0x4182d0 | 408 | 3 | menu/dialog controller + char init + shop/NPC + save path |
| 0x4c5af0 | 770 | 3 | scene-event dispatch (narrative scripting) + party/inventory |
| 0x49a050 | 13 | 2 | tile/sprite grid render + spell fx + battle UI + palette |
| 0x5a4760 | 16 | 2 | launcher config parse + spell fx + bitmap/ZDD render + RNG |
| 0x5ba3a0 | 16 | 2 | launcher config parse + spell fx + bitmap/ZDD render + RNG |
| 0x5b6ec0 | 21 | 2 | launcher config parse + spell fx + bitmap/ZDD render + RNG |
| 0x5ba3d0 | 22 | 2 | launcher config parse + spell fx + bitmap/ZDD render + RNG |
| 0x5aff00 | 26 | 2 | launcher config parse + spell fx + bitmap/ZDD render + RNG |
| 0x40bb70 | 30 | 2 | object-pool ctor + game-loop FSM + font/glyph + msg fmt |
| 0x4e61a0 | 33 | 2 | narrative scene FSM + dungeon/encounter driver (sparse) |
| 0x491740 | 38 | 2 | tile/sprite grid render + spell fx + battle UI + palette |
| 0x479e40 | 41 | 2 | battle phase controller + NPC AI + particle fx + damage UI |
| 0x494e10 | 66 | 2 | tile/sprite grid render + spell fx + battle UI + palette |
| 0x5ba650 | 78 | 2 | launcher config parse + spell fx + bitmap/ZDD render + RNG |
| 0x5ba4c0 | 84 | 2 | launcher config parse + spell fx + bitmap/ZDD render + RNG |
| 0x41bb80 | 95 | 2 | menu/dialog controller + char init + shop/NPC + save path |
| 0x5ba2d0 | 95 | 2 | launcher config parse + spell fx + bitmap/ZDD render + RNG |
| 0x411ec0 | 118 | 2 | menu/dialog controller + char init + shop/NPC + save path |
| 0x562a70 | 127 | 2 | title + gameplay scene runners + engine init + options + input init |
| 0x579860 | 131 | 2 | master sprite-group register + audio/music init + anim pump |
| 0x4118b0 | 134 | 2 | menu/dialog controller + char init + shop/NPC + save path |
| 0x43c920 | 143 | 2 | battle scenario init + turn engine + input poll + save mgr |
| 0x579680 | 191 | 2 | master sprite-group register + audio/music init + anim pump |

## Full frontier by address band

### object-pool ctor + game-loop FSM + font/glyph + msg fmt (30)

| VA | size | ported callers | unported deps | leaf |
|----|-----:|---------------:|--------------:|:----:|
| 0x40a5d0 | 568 | 6 | 5 |  |
| 0x40fe00 | 596 | 4 | 5 |  |
| 0x409090 | 67 | 3 | 0 | ✓ |
| 0x40ac90 | 834 | 3 | 5 |  |
| 0x40bb70 | 30 | 2 | 0 | ✓ |
| 0x4071f0 | 196 | 2 | 0 | ✓ |
| 0x407f30 | 453 | 2 | 0 | ✓ |
| 0x40f800 | 511 | 2 | 0 | ✓ |
| 0x408dc0 | 714 | 2 | 5 |  |
| 0x4051d0 | 3244 | 2 | 0 | ✓ |
| 0x40c380 | 5077 | 2 | 88 |  |
| 0x4034f0 | 7330 | 2 | 0 | ✓ |
| 0x402f20 | 21 | 1 | 0 | ✓ |
| 0x40f5a0 | 25 | 1 | 0 | ✓ |
| 0x402060 | 62 | 1 | 1 |  |
| 0x40fdb0 | 70 | 1 | 0 | ✓ |
| 0x4022d0 | 88 | 1 | 0 | ✓ |
| 0x40e2f0 | 100 | 1 | 0 | ✓ |
| 0x408d40 | 124 | 1 | 0 | ✓ |
| 0x401e60 | 232 | 1 | 0 | ✓ |
| 0x40df40 | 374 | 1 | 0 | ✓ |
| 0x402330 | 431 | 1 | 0 | ✓ |
| 0x406500 | 454 | 1 | 2 |  |
| 0x402d40 | 470 | 1 | 0 | ✓ |
| 0x4020a0 | 503 | 1 | 1 |  |
| 0x406210 | 555 | 1 | 1 |  |
| 0x407500 | 886 | 1 | 5 |  |
| 0x409520 | 1093 | 1 | 1 |  |
| 0x408730 | 1114 | 1 | 17 |  |
| 0x4017d0 | 1175 | 1 | 2 |  |

### menu/dialog controller + char init + shop/NPC + save path (34)

| VA | size | ported callers | unported deps | leaf |
|----|-----:|---------------:|--------------:|:----:|
| 0x41bbe0 | 312 | 4 | 0 | ✓ |
| 0x413b20 | 68 | 3 | 0 | ✓ |
| 0x412d30 | 117 | 3 | 0 | ✓ |
| 0x4124d0 | 146 | 3 | 0 | ✓ |
| 0x4182d0 | 408 | 3 | 0 | ✓ |
| 0x41bb80 | 95 | 2 | 0 | ✓ |
| 0x411ec0 | 118 | 2 | 0 | ✓ |
| 0x4118b0 | 134 | 2 | 0 | ✓ |
| 0x412c40 | 227 | 2 | 0 | ✓ |
| 0x41a890 | 3030 | 2 | 6 |  |
| 0x41e2f0 | 66 | 1 | 0 | ✓ |
| 0x41dc40 | 75 | 1 | 0 | ✓ |
| 0x412330 | 98 | 1 | 0 | ✓ |
| 0x419c00 | 110 | 1 | 0 | ✓ |
| 0x416420 | 115 | 1 | 0 | ✓ |
| 0x41e440 | 125 | 1 | 1 |  |
| 0x41a7f0 | 152 | 1 | 0 | ✓ |
| 0x413760 | 173 | 1 | 0 | ✓ |
| 0x41ec20 | 181 | 1 | 0 | ✓ |
| 0x415860 | 198 | 1 | 0 | ✓ |
| 0x41f0e0 | 280 | 1 | 1 |  |
| 0x415e20 | 320 | 1 | 0 | ✓ |
| 0x41ece0 | 369 | 1 | 1 |  |
| 0x41cfd0 | 438 | 1 | 0 | ✓ |
| 0x413810 | 444 | 1 | 3 |  |
| 0x416570 | 547 | 1 | 3 |  |
| 0x4134f0 | 613 | 1 | 0 | ✓ |
| 0x411c50 | 623 | 1 | 2 |  |
| 0x41d190 | 779 | 1 | 0 | ✓ |
| 0x41dc90 | 890 | 1 | 1 |  |
| 0x41e600 | 907 | 1 | 1 |  |
| 0x41d780 | 942 | 1 | 5 |  |
| 0x41b670 | 1295 | 1 | 6 |  |
| 0x410610 | 3103 | 1 | 1 |  |

### scene/level init + entity spawn + def-by-id lookup (1)

| VA | size | ported callers | unported deps | leaf |
|----|-----:|---------------:|--------------:|:----:|
| 0x429d00 | 68 | 1 | 0 | ✓ |

### battle scenario init + turn engine + input poll + save mgr (8)

| VA | size | ported callers | unported deps | leaf |
|----|-----:|---------------:|--------------:|:----:|
| 0x43c920 | 143 | 2 | 0 | ✓ |
| 0x439680 | 9 | 1 | 1 |  |
| 0x43c610 | 52 | 1 | 0 | ✓ |
| 0x43c170 | 53 | 1 | 0 | ✓ |
| 0x439510 | 67 | 1 | 0 | ✓ |
| 0x43c9b0 | 130 | 1 | 0 | ✓ |
| 0x438970 | 235 | 1 | 1 |  |
| 0x43c650 | 696 | 1 | 1 |  |

### entity per-frame FSM + action handlers + dialog + skills (18)

| VA | size | ported callers | unported deps | leaf |
|----|-----:|---------------:|--------------:|:----:|
| 0x445ce0 | 67 | 1 | 0 | ✓ |
| 0x445e50 | 246 | 1 | 0 | ✓ |
| 0x4466b0 | 276 | 1 | 1 |  |
| 0x44bf60 | 751 | 1 | 8 |  |
| 0x44bc10 | 819 | 1 | 6 |  |
| 0x441f50 | 1284 | 1 | 1 |  |
| 0x44a200 | 1338 | 1 | 12 |  |
| 0x44d2e0 | 1528 | 1 | 9 |  |
| 0x4484b0 | 1894 | 1 | 8 |  |
| 0x44d8f0 | 2164 | 1 | 9 |  |
| 0x448dc0 | 2221 | 1 | 15 |  |
| 0x449810 | 2221 | 1 | 9 |  |
| 0x44b160 | 2523 | 1 | 7 |  |
| 0x44a740 | 2552 | 1 | 11 |  |
| 0x44c250 | 2972 | 1 | 14 |  |
| 0x44e8d0 | 3428 | 1 | 10 |  |
| 0x44f690 | 3586 | 1 | 22 |  |
| 0x446900 | 3869 | 1 | 17 |  |

### master dialogue runner + action/frame dispatch + sprite batch (17)

| VA | size | ported callers | unported deps | leaf |
|----|-----:|---------------:|--------------:|:----:|
| 0x468b10 | 7214 | 2 | 6 |  |
| 0x467260 | 208 | 1 | 0 | ✓ |
| 0x4675c0 | 387 | 1 | 0 | ✓ |
| 0x467750 | 406 | 1 | 0 | ✓ |
| 0x4512d0 | 1012 | 1 | 7 |  |
| 0x450810 | 1485 | 1 | 8 |  |
| 0x452020 | 1649 | 1 | 12 |  |
| 0x46da00 | 1742 | 1 | 9 |  |
| 0x4516e0 | 2336 | 1 | 11 |  |
| 0x455d80 | 3175 | 1 | 17 |  |
| 0x452730 | 4119 | 1 | 14 |  |
| 0x456d30 | 4581 | 1 | 5 |  |
| 0x457ff0 | 4675 | 1 | 11 |  |
| 0x4539b0 | 8699 | 1 | 19 |  |
| 0x45e830 | 9388 | 1 | 20 |  |
| 0x45a300 | 14079 | 1 | 17 |  |
| 0x461790 | 19423 | 1 | 30 |  |

### battle phase controller + NPC AI + particle fx + damage UI (17)

| VA | size | ported callers | unported deps | leaf |
|----|-----:|---------------:|--------------:|:----:|
| 0x479e40 | 41 | 2 | 0 | ✓ |
| 0x47adb0 | 79 | 2 | 1 |  |
| 0x479e10 | 38 | 1 | 0 | ✓ |
| 0x47a300 | 278 | 1 | 0 | ✓ |
| 0x47da60 | 329 | 1 | 0 | ✓ |
| 0x47a7f0 | 376 | 1 | 0 | ✓ |
| 0x47de50 | 446 | 1 | 0 | ✓ |
| 0x47eae0 | 469 | 1 | 0 | ✓ |
| 0x47e010 | 504 | 1 | 0 | ✓ |
| 0x47a420 | 685 | 1 | 0 | ✓ |
| 0x47d790 | 710 | 1 | 0 | ✓ |
| 0x47e7c0 | 799 | 1 | 0 | ✓ |
| 0x47f670 | 861 | 1 | 0 | ✓ |
| 0x47f9d0 | 874 | 1 | 0 | ✓ |
| 0x479f90 | 875 | 1 | 1 |  |
| 0x47ecd0 | 2303 | 1 | 7 |  |
| 0x4710c0 | 20681 | 1 | 37 |  |

### char anim + hit-test/knockback + GDI glyph + sfx trigger (26)

| VA | size | ported callers | unported deps | leaf |
|----|-----:|---------------:|--------------:|:----:|
| 0x482380 | 217 | 1 | 0 | ✓ |
| 0x48e100 | 246 | 1 | 0 | ✓ |
| 0x482240 | 310 | 1 | 0 | ✓ |
| 0x48c6b0 | 359 | 1 | 0 | ✓ |
| 0x4854e0 | 388 | 1 | 0 | ✓ |
| 0x485350 | 391 | 1 | 0 | ✓ |
| 0x483de0 | 446 | 1 | 2 |  |
| 0x482e10 | 501 | 1 | 7 |  |
| 0x482880 | 532 | 1 | 5 |  |
| 0x482460 | 536 | 1 | 5 |  |
| 0x48d710 | 550 | 1 | 0 | ✓ |
| 0x483b50 | 600 | 1 | 1 |  |
| 0x484530 | 740 | 1 | 3 |  |
| 0x483fb0 | 784 | 1 | 8 |  |
| 0x485990 | 793 | 1 | 6 |  |
| 0x482ac0 | 844 | 1 | 3 |  |
| 0x48cb90 | 1005 | 1 | 0 | ✓ |
| 0x484ea0 | 1029 | 1 | 1 |  |
| 0x4834f0 | 1062 | 1 | 2 |  |
| 0x481310 | 1144 | 1 | 4 |  |
| 0x4878b0 | 1289 | 1 | 5 |  |
| 0x48da70 | 1672 | 1 | 0 | ✓ |
| 0x481ac0 | 1874 | 1 | 7 |  |
| 0x48ef40 | 4389 | 1 | 7 |  |
| 0x487dc0 | 5082 | 1 | 10 |  |
| 0x4895c0 | 8605 | 1 | 13 |  |

### tile/sprite grid render + spell fx + battle UI + palette (32)

| VA | size | ported callers | unported deps | leaf |
|----|-----:|---------------:|--------------:|:----:|
| 0x49a050 | 13 | 2 | 0 | ✓ |
| 0x491740 | 38 | 2 | 0 | ✓ |
| 0x494e10 | 66 | 2 | 0 | ✓ |
| 0x49bf90 | 424 | 2 | 2 |  |
| 0x496170 | 42 | 1 | 0 | ✓ |
| 0x4961a0 | 63 | 1 | 0 | ✓ |
| 0x49a2f0 | 76 | 1 | 0 | ✓ |
| 0x4961e0 | 95 | 1 | 0 | ✓ |
| 0x496240 | 95 | 1 | 0 | ✓ |
| 0x495dc0 | 119 | 1 | 0 | ✓ |
| 0x499070 | 142 | 1 | 0 | ✓ |
| 0x49abd0 | 168 | 1 | 0 | ✓ |
| 0x49cd70 | 254 | 1 | 2 |  |
| 0x49a340 | 291 | 1 | 0 | ✓ |
| 0x498820 | 319 | 1 | 0 | ✓ |
| 0x495fe0 | 394 | 1 | 0 | ✓ |
| 0x495e40 | 411 | 1 | 0 | ✓ |
| 0x4963a0 | 446 | 1 | 1 |  |
| 0x49cb60 | 527 | 1 | 1 |  |
| 0x49d6e0 | 590 | 1 | 0 | ✓ |
| 0x49a470 | 622 | 1 | 1 |  |
| 0x49a060 | 651 | 1 | 3 |  |
| 0x49ac80 | 664 | 1 | 0 | ✓ |
| 0x491820 | 696 | 1 | 0 | ✓ |
| 0x497c20 | 788 | 1 | 1 |  |
| 0x49c2f0 | 808 | 1 | 6 |  |
| 0x496560 | 1028 | 1 | 2 |  |
| 0x4975e0 | 1054 | 1 | 2 |  |
| 0x498960 | 1449 | 1 | 0 | ✓ |
| 0x497f40 | 1750 | 1 | 3 |  |
| 0x496ec0 | 1756 | 1 | 2 |  |
| 0x49af40 | 3313 | 1 | 4 |  |

### scene-event dispatch (narrative scripting) + party/inventory (26)

| VA | size | ported callers | unported deps | leaf |
|----|-----:|---------------:|--------------:|:----:|
| 0x4c57f0 | 50 | 3 | 0 | ✓ |
| 0x4c5af0 | 770 | 3 | 0 | ✓ |
| 0x4c6830 | 337 | 2 | 0 | ✓ |
| 0x4c63a0 | 391 | 2 | 3 |  |
| 0x4c6550 | 618 | 2 | 5 |  |
| 0x4c5830 | 694 | 2 | 0 | ✓ |
| 0x4a5f80 | 10 | 1 | 0 | ✓ |
| 0x4a5f90 | 11 | 1 | 0 | ✓ |
| 0x4a5f30 | 20 | 1 | 0 | ✓ |
| 0x4cca50 | 25 | 1 | 0 | ✓ |
| 0x4cc080 | 28 | 1 | 0 | ✓ |
| 0x4cc0a0 | 28 | 1 | 0 | ✓ |
| 0x4cc0c0 | 28 | 1 | 0 | ✓ |
| 0x4cc010 | 31 | 1 | 0 | ✓ |
| 0x4cc060 | 31 | 1 | 0 | ✓ |
| 0x4a5e20 | 32 | 1 | 1 |  |
| 0x4a5e70 | 42 | 1 | 1 |  |
| 0x4a5f50 | 45 | 1 | 0 | ✓ |
| 0x4cc030 | 45 | 1 | 0 | ✓ |
| 0x4a5ee0 | 72 | 1 | 1 |  |
| 0x4cc0e0 | 360 | 1 | 5 |  |
| 0x4c6990 | 419 | 1 | 0 | ✓ |
| 0x4cca70 | 453 | 1 | 1 |  |
| 0x4c6160 | 574 | 1 | 2 |  |
| 0x4cc5a0 | 628 | 1 | 0 | ✓ |
| 0x4c5e00 | 778 | 1 | 3 |  |

### narrative scene FSM + dungeon/encounter driver (sparse) (11)

| VA | size | ported callers | unported deps | leaf |
|----|-----:|---------------:|--------------:|:----:|
| 0x4e1950 | 135 | 3 | 0 | ✓ |
| 0x4e61a0 | 33 | 2 | 0 | ✓ |
| 0x4f69c0 | 25 | 1 | 1 |  |
| 0x4f69e0 | 33 | 1 | 0 | ✓ |
| 0x4e6030 | 54 | 1 | 0 | ✓ |
| 0x4d64b0 | 62 | 1 | 1 |  |
| 0x4e6070 | 299 | 1 | 2 |  |
| 0x4ee490 | 497 | 1 | 7 |  |
| 0x52fd80 | 767 | 1 | 0 | ✓ |
| 0x4e61d0 | 817 | 1 | 2 |  |
| 0x4e6520 | 817 | 1 | 2 |  |

### cutscene dispatcher + sprite copy + tilemap collision + camera (16)

| VA | size | ported callers | unported deps | leaf |
|----|-----:|---------------:|--------------:|:----:|
| 0x556e50 | 88 | 3 | 0 | ✓ |
| 0x55cfc0 | 358 | 2 | 1 |  |
| 0x5534d0 | 76 | 1 | 0 | ✓ |
| 0x5556d0 | 179 | 1 | 0 | ✓ |
| 0x55f550 | 199 | 1 | 0 | ✓ |
| 0x54e4f0 | 207 | 1 | 0 | ✓ |
| 0x552f60 | 590 | 1 | 4 |  |
| 0x54ed80 | 592 | 1 | 4 |  |
| 0x552c70 | 748 | 1 | 4 |  |
| 0x54bfb0 | 801 | 1 | 3 |  |
| 0x55f790 | 1558 | 1 | 0 | ✓ |
| 0x553ec0 | 1681 | 1 | 5 |  |
| 0x554570 | 1724 | 1 | 7 |  |
| 0x54f140 | 2094 | 1 | 3 |  |
| 0x554c40 | 2187 | 1 | 5 |  |
| 0x553520 | 2458 | 1 | 4 |  |

### title + gameplay scene runners + engine init + options + input init (42)

| VA | size | ported callers | unported deps | leaf |
|----|-----:|---------------:|--------------:|:----:|
| 0x566250 | 169 | 3 | 1 |  |
| 0x562a70 | 127 | 2 | 0 | ✓ |
| 0x56df10 | 203 | 2 | 0 | ✓ |
| 0x562d50 | 239 | 2 | 0 | ✓ |
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
| 0x5614e0 | 113 | 1 | 0 | ✓ |
| 0x56cb90 | 113 | 1 | 0 | ✓ |
| 0x565840 | 115 | 1 | 0 | ✓ |
| 0x561150 | 126 | 1 | 0 | ✓ |
| 0x5611d0 | 173 | 1 | 0 | ✓ |
| 0x5646c0 | 180 | 1 | 3 |  |
| 0x56d620 | 237 | 1 | 3 |  |
| 0x56dd80 | 254 | 1 | 0 | ✓ |
| 0x56cc10 | 267 | 1 | 0 | ✓ |
| 0x56daa0 | 267 | 1 | 0 | ✓ |
| 0x56dfe0 | 417 | 1 | 0 | ✓ |
| 0x562ba0 | 432 | 1 | 4 |  |
| 0x5642e0 | 440 | 1 | 7 |  |
| 0x5644a0 | 483 | 1 | 6 |  |
| 0x565b00 | 528 | 1 | 7 |  |
| 0x566350 | 540 | 1 | 3 |  |
| 0x568780 | 554 | 1 | 2 |  |
| 0x5658c0 | 565 | 1 | 9 |  |
| 0x568b40 | 659 | 1 | 7 |  |
| 0x560e90 | 700 | 1 | 0 | ✓ |
| 0x568de0 | 998 | 1 | 13 |  |
| 0x56a670 | 1011 | 1 | 18 |  |
| 0x561d20 | 1259 | 1 | 11 |  |
| 0x5624c0 | 1355 | 1 | 11 |  |

### master sprite-group register + audio/music init + anim pump (27)

| VA | size | ported callers | unported deps | leaf |
|----|-----:|---------------:|--------------:|:----:|
| 0x579860 | 131 | 2 | 0 | ✓ |
| 0x579680 | 191 | 2 | 0 | ✓ |
| 0x579740 | 285 | 2 | 2 |  |
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
| 0x5798f0 | 259 | 1 | 0 | ✓ |
| 0x58d0b0 | 337 | 1 | 0 | ✓ |
| 0x58d210 | 403 | 1 | 1 |  |
| 0x58e170 | 441 | 1 | 2 |  |
| 0x583c90 | 580 | 1 | 0 | ✓ |
| 0x58e330 | 873 | 1 | 0 | ✓ |
| 0x583fe0 | 979 | 1 | 13 |  |
| 0x58f360 | 3030 | 1 | 27 |  |
| 0x58d460 | 3341 | 1 | 3 |  |

### inventory/menu + audio cue mgr + render dispatch + scene load (6)

| VA | size | ported callers | unported deps | leaf |
|----|-----:|---------------:|--------------:|:----:|
| 0x59e1a0 | 34 | 1 | 0 | ✓ |
| 0x59db70 | 227 | 1 | 0 | ✓ |
| 0x59cfc0 | 405 | 1 | 2 |  |
| 0x59ec30 | 531 | 1 | 5 |  |
| 0x59e230 | 924 | 1 | 19 |  |
| 0x59e5d0 | 1033 | 1 | 14 |  |

### launcher config parse + spell fx + bitmap/ZDD render + RNG (84)

| VA | size | ported callers | unported deps | leaf |
|----|-----:|---------------:|--------------:|:----:|
| 0x5bb870 | 16 | 22 | 0 | ✓ |
| 0x5bb8c0 | 31 | 18 | 0 | ✓ |
| 0x5bb890 | 40 | 16 | 0 | ✓ |
| 0x5bb880 | 16 | 15 | 0 | ✓ |
| 0x5bb8e0 | 9 | 10 | 0 | ✓ |
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
| 0x5bc0c0 | 143 | 3 | 1 |  |
| 0x5a4760 | 16 | 2 | 0 | ✓ |
| 0x5ba3a0 | 16 | 2 | 0 | ✓ |
| 0x5b6ec0 | 21 | 2 | 0 | ✓ |
| 0x5ba3d0 | 22 | 2 | 0 | ✓ |
| 0x5aff00 | 26 | 2 | 0 | ✓ |
| 0x5bcb30 | 70 | 2 | 1 |  |
| 0x5ba650 | 78 | 2 | 0 | ✓ |
| 0x5ba4c0 | 84 | 2 | 0 | ✓ |
| 0x5ba2d0 | 95 | 2 | 0 | ✓ |
| 0x5ba120 | 154 | 2 | 1 |  |
| 0x5ba3f0 | 17 | 1 | 0 | ✓ |
| 0x5ba520 | 21 | 1 | 0 | ✓ |
| 0x5a3bf0 | 27 | 1 | 0 | ✓ |
| 0x5b6580 | 28 | 1 | 0 | ✓ |
| 0x5a4460 | 30 | 1 | 1 |  |
| 0x5b0260 | 30 | 1 | 0 | ✓ |
| 0x5b5a90 | 33 | 1 | 0 | ✓ |
| 0x5a3c10 | 37 | 1 | 0 | ✓ |
| 0x5b64f0 | 38 | 1 | 0 | ✓ |
| 0x5b5a60 | 42 | 1 | 0 | ✓ |
| 0x5a3de0 | 49 | 1 | 0 | ✓ |
| 0x5b5d50 | 52 | 1 | 0 | ✓ |
| 0x5b9450 | 52 | 1 | 0 | ✓ |
| 0x5bbb10 | 56 | 1 | 0 | ✓ |
| 0x5b6300 | 60 | 1 | 0 | ✓ |
| 0x5a3bb0 | 62 | 1 | 0 | ✓ |
| 0x5b0390 | 66 | 1 | 0 | ✓ |
| 0x5b68f0 | 67 | 1 | 2 |  |
| 0x5b61b0 | 68 | 1 | 1 |  |
| 0x5b6200 | 78 | 1 | 1 |  |
| 0x5b0d70 | 82 | 1 | 0 | ✓ |
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
| 0x5a4480 | 312 | 1 | 1 |  |
| 0x5b0ee0 | 331 | 1 | 0 | ✓ |
| 0x5a3c40 | 404 | 1 | 0 | ✓ |
| 0x5affe0 | 409 | 1 | 0 | ✓ |
| 0x5a3e20 | 455 | 1 | 0 | ✓ |
| 0x5b6bc0 | 491 | 1 | 8 |  |
| 0x5a3670 | 499 | 1 | 1 |  |
| 0x5a3ff0 | 516 | 1 | 4 |  |
| 0x5b10d0 | 520 | 1 | 3 |  |
| 0x5bbeb0 | 527 | 1 | 1 |  |
| 0x5b7970 | 539 | 1 | 0 | ✓ |
| 0x5b5af0 | 595 | 1 | 0 | ✓ |
| 0x5afc90 | 623 | 1 | 8 |  |
| 0x5b5600 | 701 | 1 | 1 |  |
| 0x5b4d00 | 792 | 1 | 7 |  |
| 0x5a3870 | 828 | 1 | 0 | ✓ |
| 0x5bc7c0 | 880 | 1 | 3 |  |
| 0x5b0890 | 961 | 1 | 0 | ✓ |
| 0x5b0450 | 1075 | 1 | 0 | ✓ |

