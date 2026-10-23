# OpenSummoners — port frontier (what to port next)

> **DERIVED FILE** — `python3 tools/gen_port_ledger.py && python3 tools/gen_frontier.py`.

The unported, non-thunk, engine-proper functions already **called by ported code** — the natural next chips. A **leaf** has all of its
own engine callees ported, so it can land today with zero new dependencies. Sorted by how many ported callers want it.

For the *forward* port path (the title-menu scene runner and what it calls) and the semantic milestone order, see `ROADMAP.md` — some of that path isn't yet reachable from ported code so won't appear here.

- frontier functions: **180**
- of those, zero-dependency **leaves: 124** (recommended order below)

## Leaf shortlist — portable today (top 40 by ported-caller count)

| VA | size | ported callers | band |
|----|-----:|---------------:|------|
| 0x556e50 | 88 | 4 | cutscene dispatcher + sprite copy + tilemap collision + camera |
| 0x40ac90 | 834 | 4 | object-pool ctor + game-loop FSM + font/glyph + msg fmt |
| 0x407f30 | 453 | 3 | object-pool ctor + game-loop FSM + font/glyph + msg fmt |
| 0x55fdb0 | 461 | 3 | cutscene dispatcher + sprite copy + tilemap collision + camera |
| 0x4c5af0 | 770 | 3 | scene-event dispatch (narrative scripting) + party/inventory |
| 0x5a4760 | 16 | 2 | launcher config parse + spell fx + bitmap/ZDD render + RNG |
| 0x5aff00 | 26 | 2 | launcher config parse + spell fx + bitmap/ZDD render + RNG |
| 0x579860 | 131 | 2 | master sprite-group register + audio/music init + anim pump |
| 0x5646c0 | 180 | 2 | title + gameplay scene runners + engine init + options + input init |
| 0x579680 | 191 | 2 | master sprite-group register + audio/music init + anim pump |
| 0x579740 | 285 | 2 | master sprite-group register + audio/music init + anim pump |
| 0x4cc0e0 | 360 | 2 | scene-event dispatch (narrative scripting) + party/inventory |
| 0x4190d0 | 470 | 2 | menu/dialog controller + char init + shop/NPC + save path |
| 0x5a3670 | 499 | 2 | launcher config parse + spell fx + bitmap/ZDD render + RNG |
| 0x409150 | 881 | 2 | object-pool ctor + game-loop FSM + font/glyph + msg fmt |
| 0x4051d0 | 3244 | 2 | object-pool ctor + game-loop FSM + font/glyph + msg fmt |
| 0x4034f0 | 7330 | 2 | object-pool ctor + game-loop FSM + font/glyph + msg fmt |
| 0x584400 | 19 | 1 | master sprite-group register + audio/music init + anim pump |
| 0x5a4440 | 26 | 1 | launcher config parse + spell fx + bitmap/ZDD render + RNG |
| 0x5b0260 | 30 | 1 | launcher config parse + spell fx + bitmap/ZDD render + RNG |
| 0x5b5a90 | 33 | 1 | launcher config parse + spell fx + bitmap/ZDD render + RNG |
| 0x597480 | 37 | 1 | inventory/menu + audio cue mgr + render dispatch + scene load |
| 0x4428f0 | 41 | 1 | entity per-frame FSM + action handlers + dialog + skills |
| 0x5b5a60 | 42 | 1 | launcher config parse + spell fx + bitmap/ZDD render + RNG |
| 0x58c8d0 | 51 | 1 | master sprite-group register + audio/music init + anim pump |
| 0x5b5d50 | 52 | 1 | launcher config parse + spell fx + bitmap/ZDD render + RNG |
| 0x5b0390 | 66 | 1 | launcher config parse + spell fx + bitmap/ZDD render + RNG |
| 0x5534d0 | 76 | 1 | cutscene dispatcher + sprite copy + tilemap collision + camera |
| 0x564110 | 80 | 1 | title + gameplay scene runners + engine init + options + input init |
| 0x566300 | 80 | 1 | title + gameplay scene runners + engine init + options + input init |
| 0x5640b0 | 81 | 1 | title + gameplay scene runners + engine init + options + input init |
| 0x5b0d70 | 82 | 1 | launcher config parse + spell fx + bitmap/ZDD render + RNG |
| 0x4e6870 | 84 | 1 | narrative scene FSM + dungeon/encounter driver (sparse) |
| 0x5b5ef0 | 87 | 1 | launcher config parse + spell fx + bitmap/ZDD render + RNG |
| 0x5afb90 | 96 | 1 | launcher config parse + spell fx + bitmap/ZDD render + RNG |
| 0x41db30 | 109 | 1 | menu/dialog controller + char init + shop/NPC + save path |
| 0x5b03e0 | 112 | 1 | launcher config parse + spell fx + bitmap/ZDD render + RNG |
| 0x5b01e0 | 124 | 1 | launcher config parse + spell fx + bitmap/ZDD render + RNG |
| 0x5b0c60 | 128 | 1 | launcher config parse + spell fx + bitmap/ZDD render + RNG |
| 0x5afb00 | 129 | 1 | launcher config parse + spell fx + bitmap/ZDD render + RNG |

## Full frontier by address band

### object-pool ctor + game-loop FSM + font/glyph + msg fmt (6)

| VA | size | ported callers | unported deps | leaf |
|----|-----:|---------------:|--------------:|:----:|
| 0x40fe00 | 596 | 13 | 2 |  |
| 0x40ac90 | 834 | 4 | 0 | ✓ |
| 0x407f30 | 453 | 3 | 0 | ✓ |
| 0x409150 | 881 | 2 | 0 | ✓ |
| 0x4051d0 | 3244 | 2 | 0 | ✓ |
| 0x4034f0 | 7330 | 2 | 0 | ✓ |

### menu/dialog controller + char init + shop/NPC + save path (9)

| VA | size | ported callers | unported deps | leaf |
|----|-----:|---------------:|--------------:|:----:|
| 0x41c390 | 1919 | 10 | 3 |  |
| 0x4190d0 | 470 | 2 | 0 | ✓ |
| 0x415ae0 | 786 | 2 | 1 |  |
| 0x41db30 | 109 | 1 | 0 | ✓ |
| 0x41dba0 | 148 | 1 | 0 | ✓ |
| 0x41e4c0 | 307 | 1 | 0 | ✓ |
| 0x41b470 | 506 | 1 | 0 | ✓ |
| 0x418ec0 | 522 | 1 | 0 | ✓ |
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
| 0x43c1b0 | 302 | 1 | 0 | ✓ |
| 0x43dd30 | 728 | 1 | 0 | ✓ |
| 0x43d540 | 748 | 1 | 1 |  |
| 0x43d830 | 1254 | 1 | 0 | ✓ |
| 0x430400 | 5039 | 1 | 0 | ✓ |

### entity per-frame FSM + action handlers + dialog + skills (16)

| VA | size | ported callers | unported deps | leaf |
|----|-----:|---------------:|--------------:|:----:|
| 0x4428f0 | 41 | 1 | 0 | ✓ |
| 0x445e50 | 246 | 1 | 0 | ✓ |
| 0x441de0 | 274 | 1 | 0 | ✓ |
| 0x44bf60 | 751 | 1 | 0 | ✓ |
| 0x44bc10 | 819 | 1 | 0 | ✓ |
| 0x44a200 | 1338 | 1 | 0 | ✓ |
| 0x4484b0 | 1894 | 1 | 0 | ✓ |
| 0x44d8f0 | 2164 | 1 | 2 |  |
| 0x448dc0 | 2221 | 1 | 2 |  |
| 0x449810 | 2221 | 1 | 1 |  |
| 0x44b160 | 2523 | 1 | 0 | ✓ |
| 0x44a740 | 2552 | 1 | 0 | ✓ |
| 0x44c250 | 2972 | 1 | 1 |  |
| 0x44e8d0 | 3428 | 1 | 0 | ✓ |
| 0x44f690 | 3586 | 1 | 3 |  |
| 0x446900 | 3869 | 1 | 0 | ✓ |

### master dialogue runner + action/frame dispatch + sprite batch (10)

| VA | size | ported callers | unported deps | leaf |
|----|-----:|---------------:|--------------:|:----:|
| 0x468b10 | 7214 | 4 | 2 |  |
| 0x468970 | 172 | 1 | 0 | ✓ |
| 0x4675c0 | 387 | 1 | 0 | ✓ |
| 0x467750 | 406 | 1 | 0 | ✓ |
| 0x450810 | 1485 | 1 | 0 | ✓ |
| 0x452020 | 1649 | 1 | 1 |  |
| 0x4516e0 | 2336 | 1 | 0 | ✓ |
| 0x452730 | 4119 | 1 | 0 | ✓ |
| 0x456d30 | 4581 | 1 | 0 | ✓ |
| 0x4539b0 | 8699 | 1 | 5 |  |

### battle phase controller + NPC AI + particle fx + damage UI (3)

| VA | size | ported callers | unported deps | leaf |
|----|-----:|---------------:|--------------:|:----:|
| 0x47a300 | 278 | 1 | 0 | ✓ |
| 0x47a7f0 | 376 | 1 | 0 | ✓ |
| 0x47a420 | 685 | 1 | 0 | ✓ |

### tile/sprite grid render + spell fx + battle UI + palette (3)

| VA | size | ported callers | unported deps | leaf |
|----|-----:|---------------:|--------------:|:----:|
| 0x49d330 | 754 | 9 | 1 |  |
| 0x49bf90 | 424 | 7 | 1 |  |
| 0x49eea0 | 2332 | 2 | 1 |  |

### scene-event dispatch (narrative scripting) + party/inventory (2)

| VA | size | ported callers | unported deps | leaf |
|----|-----:|---------------:|--------------:|:----:|
| 0x4c5af0 | 770 | 3 | 0 | ✓ |
| 0x4cc0e0 | 360 | 2 | 0 | ✓ |

### narrative scene FSM + dungeon/encounter driver (sparse) (32)

| VA | size | ported callers | unported deps | leaf |
|----|-----:|---------------:|--------------:|:----:|
| 0x4e6870 | 84 | 1 | 0 | ✓ |
| 0x4e68d0 | 393 | 1 | 0 | ✓ |
| 0x523010 | 743 | 1 | 0 | ✓ |
| 0x52fd80 | 767 | 1 | 0 | ✓ |
| 0x4e61d0 | 817 | 1 | 1 |  |
| 0x4e6520 | 817 | 1 | 1 |  |
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

### cutscene dispatcher + sprite copy + tilemap collision + camera (15)

| VA | size | ported callers | unported deps | leaf |
|----|-----:|---------------:|--------------:|:----:|
| 0x556e50 | 88 | 4 | 0 | ✓ |
| 0x55fdb0 | 461 | 3 | 0 | ✓ |
| 0x5534d0 | 76 | 1 | 0 | ✓ |
| 0x5556d0 | 179 | 1 | 0 | ✓ |
| 0x54e4f0 | 207 | 1 | 0 | ✓ |
| 0x552f60 | 590 | 1 | 0 | ✓ |
| 0x54ed80 | 592 | 1 | 1 |  |
| 0x552c70 | 748 | 1 | 0 | ✓ |
| 0x553ec0 | 1681 | 1 | 0 | ✓ |
| 0x554570 | 1724 | 1 | 0 | ✓ |
| 0x54f140 | 2094 | 1 | 0 | ✓ |
| 0x554c40 | 2187 | 1 | 1 |  |
| 0x553520 | 2458 | 1 | 0 | ✓ |
| 0x546670 | 22187 | 1 | 6 |  |
| 0x5402e0 | 25033 | 1 | 3 |  |

### title + gameplay scene runners + engine init + options + input init (13)

| VA | size | ported callers | unported deps | leaf |
|----|-----:|---------------:|--------------:|:----:|
| 0x56c640 | 690 | 4 | 2 |  |
| 0x5646c0 | 180 | 2 | 0 | ✓ |
| 0x569220 | 4007 | 2 | 4 |  |
| 0x564110 | 80 | 1 | 0 | ✓ |
| 0x566300 | 80 | 1 | 0 | ✓ |
| 0x5640b0 | 81 | 1 | 0 | ✓ |
| 0x56d620 | 237 | 1 | 0 | ✓ |
| 0x56dd80 | 254 | 1 | 0 | ✓ |
| 0x56ad50 | 334 | 1 | 2 |  |
| 0x565b00 | 528 | 1 | 0 | ✓ |
| 0x568780 | 554 | 1 | 0 | ✓ |
| 0x56a670 | 1011 | 1 | 3 |  |
| 0x5624c0 | 1355 | 1 | 3 |  |

### master sprite-group register + audio/music init + anim pump (10)

| VA | size | ported callers | unported deps | leaf |
|----|-----:|---------------:|--------------:|:----:|
| 0x579860 | 131 | 2 | 0 | ✓ |
| 0x579680 | 191 | 2 | 0 | ✓ |
| 0x579740 | 285 | 2 | 0 | ✓ |
| 0x584400 | 19 | 1 | 0 | ✓ |
| 0x58c8d0 | 51 | 1 | 0 | ✓ |
| 0x583ee0 | 248 | 1 | 0 | ✓ |
| 0x5798f0 | 259 | 1 | 0 | ✓ |
| 0x584420 | 292 | 1 | 0 | ✓ |
| 0x583c90 | 580 | 1 | 0 | ✓ |
| 0x583fe0 | 979 | 1 | 8 |  |

### inventory/menu + audio cue mgr + render dispatch + scene load (11)

| VA | size | ported callers | unported deps | leaf |
|----|-----:|---------------:|--------------:|:----:|
| 0x597480 | 37 | 1 | 0 | ✓ |
| 0x5974b0 | 446 | 1 | 0 | ✓ |
| 0x596b50 | 528 | 1 | 1 |  |
| 0x59d160 | 644 | 1 | 0 | ✓ |
| 0x59e230 | 924 | 1 | 6 |  |
| 0x598d00 | 933 | 1 | 5 |  |
| 0x59e5d0 | 1033 | 1 | 6 |  |
| 0x597670 | 1060 | 1 | 0 | ✓ |
| 0x593f60 | 1114 | 1 | 1 |  |
| 0x592190 | 1715 | 1 | 1 |  |
| 0x5915c0 | 2655 | 1 | 1 |  |

### launcher config parse + spell fx + bitmap/ZDD render + RNG (31)

| VA | size | ported callers | unported deps | leaf |
|----|-----:|---------------:|--------------:|:----:|
| 0x5a4760 | 16 | 2 | 0 | ✓ |
| 0x5aff00 | 26 | 2 | 0 | ✓ |
| 0x5a3670 | 499 | 2 | 0 | ✓ |
| 0x5a4440 | 26 | 1 | 0 | ✓ |
| 0x5b0260 | 30 | 1 | 0 | ✓ |
| 0x5b5a90 | 33 | 1 | 0 | ✓ |
| 0x5b5a60 | 42 | 1 | 0 | ✓ |
| 0x5b5d50 | 52 | 1 | 0 | ✓ |
| 0x5b0390 | 66 | 1 | 0 | ✓ |
| 0x5b0d70 | 82 | 1 | 0 | ✓ |
| 0x5b5ef0 | 87 | 1 | 0 | ✓ |
| 0x5afb90 | 96 | 1 | 0 | ✓ |
| 0x5b03e0 | 112 | 1 | 0 | ✓ |
| 0x5b01e0 | 124 | 1 | 0 | ✓ |
| 0x5b0c60 | 128 | 1 | 0 | ✓ |
| 0x5afb00 | 129 | 1 | 0 | ✓ |
| 0x5b0ce0 | 131 | 1 | 0 | ✓ |
| 0x5afbf0 | 146 | 1 | 0 | ✓ |
| 0x5aff20 | 186 | 1 | 0 | ✓ |
| 0x5b0280 | 264 | 1 | 0 | ✓ |
| 0x5b0dd0 | 267 | 1 | 0 | ✓ |
| 0x5a4320 | 278 | 1 | 1 |  |
| 0x5b0ee0 | 331 | 1 | 0 | ✓ |
| 0x5affe0 | 409 | 1 | 0 | ✓ |
| 0x5b10d0 | 520 | 1 | 2 |  |
| 0x5b5af0 | 595 | 1 | 0 | ✓ |
| 0x5afc90 | 623 | 1 | 0 | ✓ |
| 0x5a3870 | 828 | 1 | 0 | ✓ |
| 0x5b0890 | 961 | 1 | 0 | ✓ |
| 0x5b0450 | 1075 | 1 | 0 | ✓ |
| 0x5b1f10 | 2345 | 1 | 2 |  |

