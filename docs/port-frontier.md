# OpenSummoners — port frontier (what to port next)

> **DERIVED FILE** — `python3 tools/gen_port_ledger.py && python3 tools/gen_frontier.py`.

The unported, non-thunk, engine-proper functions already **called by ported code** — the natural next chips. A **leaf** has all of its
own engine callees ported, so it can land today with zero new dependencies. Sorted by how many ported callers want it.

For the *forward* port path (the title-menu scene runner and what it calls) and the semantic milestone order, see `ROADMAP.md` — some of that path isn't yet reachable from ported code so won't appear here.

- frontier functions: **105**
- of those, zero-dependency **leaves: 51** (recommended order below)

## Leaf shortlist — portable today (top 40 by ported-caller count)

| VA | size | ported callers | band |
|----|-----:|---------------:|------|
| 0x5b6ec0 | 21 | 2 | launcher config parse + spell fx + bitmap/ZDD render + RNG |
| 0x417870 | 106 | 2 | menu/dialog controller + char init + shop/NPC + save path |
| 0x562a70 | 127 | 2 | title + gameplay scene runners + engine init + options + input init |
| 0x56df10 | 203 | 2 | title + gameplay scene runners + engine init + options + input init |
| 0x562d50 | 239 | 2 | title + gameplay scene runners + engine init + options + input init |
| 0x41bbe0 | 312 | 2 | menu/dialog controller + char init + shop/NPC + save path |
| 0x49a050 | 13 | 1 | tile/sprite grid render + spell fx + battle UI + palette |
| 0x5a4760 | 16 | 1 | launcher config parse + spell fx + bitmap/ZDD render + RNG |
| 0x58d090 | 18 | 1 | master sprite-group register + audio/music init + anim pump |
| 0x5aff00 | 26 | 1 | launcher config parse + spell fx + bitmap/ZDD render + RNG |
| 0x58cfc0 | 27 | 1 | master sprite-group register + audio/music init + anim pump |
| 0x587d30 | 28 | 1 | master sprite-group register + audio/music init + anim pump |
| 0x5b6580 | 28 | 1 | launcher config parse + spell fx + bitmap/ZDD render + RNG |
| 0x426f70 | 37 | 1 | scene/level init + entity spawn + def-by-id lookup |
| 0x561280 | 37 | 1 | title + gameplay scene runners + engine init + options + input init |
| 0x491740 | 38 | 1 | tile/sprite grid render + spell fx + battle UI + palette |
| 0x58d060 | 45 | 1 | master sprite-group register + audio/music init + anim pump |
| 0x5bbc60 | 45 | 1 | launcher config parse + spell fx + bitmap/ZDD render + RNG |
| 0x4022a0 | 47 | 1 | object-pool ctor + game-loop FSM + font/glyph + msg fmt |
| 0x58c8d0 | 51 | 1 | master sprite-group register + audio/music init + anim pump |
| 0x58d020 | 51 | 1 | master sprite-group register + audio/music init + anim pump |
| 0x5bbc90 | 55 | 1 | launcher config parse + spell fx + bitmap/ZDD render + RNG |
| 0x5bbb10 | 56 | 1 | launcher config parse + spell fx + bitmap/ZDD render + RNG |
| 0x412db0 | 57 | 1 | menu/dialog controller + char init + shop/NPC + save path |
| 0x5bbc20 | 57 | 1 | launcher config parse + spell fx + bitmap/ZDD render + RNG |
| 0x58cfe0 | 63 | 1 | master sprite-group register + audio/music init + anim pump |
| 0x56c430 | 64 | 1 | title + gameplay scene runners + engine init + options + input init |
| 0x587ce0 | 71 | 1 | master sprite-group register + audio/music init + anim pump |
| 0x587db0 | 76 | 1 | master sprite-group register + audio/music init + anim pump |
| 0x564110 | 80 | 1 | title + gameplay scene runners + engine init + options + input init |
| 0x58cf60 | 83 | 1 | master sprite-group register + audio/music init + anim pump |
| 0x54c970 | 84 | 1 | cutscene dispatcher + sprite copy + tilemap collision + camera |
| 0x4022d0 | 88 | 1 | object-pool ctor + game-loop FSM + font/glyph + msg fmt |
| 0x56bfd0 | 88 | 1 | title + gameplay scene runners + engine init + options + input init |
| 0x41bb80 | 95 | 1 | menu/dialog controller + char init + shop/NPC + save path |
| 0x587d50 | 96 | 1 | master sprite-group register + audio/music init + anim pump |
| 0x56cb90 | 113 | 1 | title + gameplay scene runners + engine init + options + input init |
| 0x411560 | 117 | 1 | menu/dialog controller + char init + shop/NPC + save path |
| 0x417bc0 | 123 | 1 | menu/dialog controller + char init + shop/NPC + save path |
| 0x43c9b0 | 130 | 1 | battle scenario init + turn engine + input poll + save mgr |

## Full frontier by address band

### object-pool ctor + game-loop FSM + font/glyph + msg fmt (8)

| VA | size | ported callers | unported deps | leaf |
|----|-----:|---------------:|--------------:|:----:|
| 0x40a5d0 | 568 | 3 | 5 |  |
| 0x4022a0 | 47 | 1 | 0 | ✓ |
| 0x4022d0 | 88 | 1 | 0 | ✓ |
| 0x40fe00 | 596 | 1 | 5 |  |
| 0x40b8f0 | 626 | 1 | 3 |  |
| 0x40fa00 | 800 | 1 | 3 |  |
| 0x4017d0 | 1175 | 1 | 1 |  |
| 0x40c380 | 5077 | 1 | 100 |  |

### menu/dialog controller + char init + shop/NPC + save path (13)

| VA | size | ported callers | unported deps | leaf |
|----|-----:|---------------:|--------------:|:----:|
| 0x417870 | 106 | 2 | 0 | ✓ |
| 0x41bbe0 | 312 | 2 | 0 | ✓ |
| 0x412db0 | 57 | 1 | 0 | ✓ |
| 0x41bb80 | 95 | 1 | 0 | ✓ |
| 0x411560 | 117 | 1 | 0 | ✓ |
| 0x417bc0 | 123 | 1 | 0 | ✓ |
| 0x4118b0 | 134 | 1 | 0 | ✓ |
| 0x4182d0 | 408 | 1 | 0 | ✓ |
| 0x411390 | 413 | 1 | 5 |  |
| 0x41dc90 | 890 | 1 | 2 |  |
| 0x41e600 | 907 | 1 | 5 |  |
| 0x417c40 | 1625 | 1 | 1 |  |
| 0x41a890 | 3030 | 1 | 6 |  |

### scene/level init + entity spawn + def-by-id lookup (1)

| VA | size | ported callers | unported deps | leaf |
|----|-----:|---------------:|--------------:|:----:|
| 0x426f70 | 37 | 1 | 0 | ✓ |

### battle scenario init + turn engine + input poll + save mgr (4)

| VA | size | ported callers | unported deps | leaf |
|----|-----:|---------------:|--------------:|:----:|
| 0x43c9b0 | 130 | 1 | 0 | ✓ |
| 0x43e140 | 266 | 1 | 4 |  |
| 0x43e3a0 | 516 | 1 | 1 |  |
| 0x43c2e0 | 795 | 1 | 1 |  |

### entity per-frame FSM + action handlers + dialog + skills (1)

| VA | size | ported callers | unported deps | leaf |
|----|-----:|---------------:|--------------:|:----:|
| 0x4467d0 | 289 | 1 | 2 |  |

### battle phase controller + NPC AI + particle fx + damage UI (1)

| VA | size | ported callers | unported deps | leaf |
|----|-----:|---------------:|--------------:|:----:|
| 0x47b7c0 | 163 | 1 | 0 | ✓ |

### tile/sprite grid render + spell fx + battle UI + palette (2)

| VA | size | ported callers | unported deps | leaf |
|----|-----:|---------------:|--------------:|:----:|
| 0x49a050 | 13 | 1 | 0 | ✓ |
| 0x491740 | 38 | 1 | 0 | ✓ |

### cutscene dispatcher + sprite copy + tilemap collision + camera (3)

| VA | size | ported callers | unported deps | leaf |
|----|-----:|---------------:|--------------:|:----:|
| 0x54c970 | 84 | 1 | 0 | ✓ |
| 0x54bfb0 | 801 | 1 | 3 |  |
| 0x54c640 | 812 | 1 | 2 |  |

### title + gameplay scene runners + engine init + options + input init (19)

| VA | size | ported callers | unported deps | leaf |
|----|-----:|---------------:|--------------:|:----:|
| 0x562a70 | 127 | 2 | 0 | ✓ |
| 0x56df10 | 203 | 2 | 0 | ✓ |
| 0x562d50 | 239 | 2 | 0 | ✓ |
| 0x561280 | 37 | 1 | 0 | ✓ |
| 0x56c430 | 64 | 1 | 0 | ✓ |
| 0x56de80 | 67 | 1 | 1 |  |
| 0x564110 | 80 | 1 | 0 | ✓ |
| 0x5640b0 | 81 | 1 | 1 |  |
| 0x562af0 | 82 | 1 | 1 |  |
| 0x56bfd0 | 88 | 1 | 0 | ✓ |
| 0x56cb90 | 113 | 1 | 0 | ✓ |
| 0x566250 | 169 | 1 | 2 |  |
| 0x56dc20 | 344 | 1 | 1 |  |
| 0x564160 | 376 | 1 | 17 |  |
| 0x56c930 | 607 | 1 | 4 |  |
| 0x568de0 | 998 | 1 | 19 |  |
| 0x56a670 | 1011 | 1 | 22 |  |
| 0x5624c0 | 1355 | 1 | 12 |  |
| 0x56cd20 | 2275 | 1 | 14 |  |

### master sprite-group register + audio/music init + anim pump (28)

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
| 0x58ca80 | 167 | 1 | 0 | ✓ |
| 0x5878a0 | 207 | 1 | 0 | ✓ |
| 0x58e6a0 | 218 | 1 | 1 |  |
| 0x583ee0 | 248 | 1 | 0 | ✓ |
| 0x58d0b0 | 337 | 1 | 4 |  |
| 0x58c910 | 347 | 1 | 0 | ✓ |
| 0x58d210 | 403 | 1 | 1 |  |
| 0x58e170 | 441 | 1 | 2 |  |
| 0x583c90 | 580 | 1 | 0 | ✓ |
| 0x587970 | 866 | 1 | 4 |  |
| 0x58e330 | 873 | 1 | 0 | ✓ |
| 0x583fe0 | 979 | 1 | 19 |  |
| 0x58cb30 | 1065 | 1 | 0 | ✓ |
| 0x58e780 | 1823 | 1 | 7 |  |
| 0x58f360 | 3030 | 1 | 45 |  |
| 0x58d460 | 3341 | 1 | 5 |  |
| 0x5752e0 | 17310 | 1 | 9 |  |

### inventory/menu + audio cue mgr + render dispatch + scene load (4)

| VA | size | ported callers | unported deps | leaf |
|----|-----:|---------------:|--------------:|:----:|
| 0x59db70 | 227 | 1 | 0 | ✓ |
| 0x59ec30 | 531 | 1 | 6 |  |
| 0x59e230 | 924 | 1 | 20 |  |
| 0x59e5d0 | 1033 | 1 | 15 |  |

### launcher config parse + spell fx + bitmap/ZDD render + RNG (21)

| VA | size | ported callers | unported deps | leaf |
|----|-----:|---------------:|--------------:|:----:|
| 0x5b6ec0 | 21 | 2 | 0 | ✓ |
| 0x5ba120 | 154 | 2 | 1 |  |
| 0x5a4760 | 16 | 1 | 0 | ✓ |
| 0x5aff00 | 26 | 1 | 0 | ✓ |
| 0x5b6580 | 28 | 1 | 0 | ✓ |
| 0x5bbc60 | 45 | 1 | 0 | ✓ |
| 0x5bbc90 | 55 | 1 | 0 | ✓ |
| 0x5bbb10 | 56 | 1 | 0 | ✓ |
| 0x5bbc20 | 57 | 1 | 0 | ✓ |
| 0x5bcb30 | 70 | 1 | 1 |  |
| 0x5bbcd0 | 72 | 1 | 1 |  |
| 0x5b65b0 | 127 | 1 | 2 |  |
| 0x5b6db0 | 129 | 1 | 1 |  |
| 0x5bb2f0 | 140 | 1 | 2 |  |
| 0x5bb250 | 150 | 1 | 2 |  |
| 0x5b9cf0 | 159 | 1 | 3 |  |
| 0x5b9fc0 | 164 | 1 | 3 |  |
| 0x5baed0 | 193 | 1 | 3 |  |
| 0x5b10d0 | 520 | 1 | 3 |  |
| 0x5bbeb0 | 527 | 1 | 1 |  |
| 0x5bcb80 | 611 | 1 | 3 |  |

