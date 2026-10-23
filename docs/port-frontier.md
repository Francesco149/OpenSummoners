# OpenSummoners — port frontier (what to port next)

> **DERIVED FILE** — `python3 tools/gen_port_ledger.py && python3 tools/gen_frontier.py`.

The unported, non-thunk, engine-proper functions already **called by ported code** — the natural next chips. A **leaf** has all of its
own engine callees ported, so it can land today with zero new dependencies. Sorted by how many ported callers want it.

For the *forward* port path (the title-menu scene runner and what it calls) and the semantic milestone order, see `ROADMAP.md` — some of that path isn't yet reachable from ported code so won't appear here.

- frontier functions: **3**
- of those, zero-dependency **leaves: 3** (recommended order below)

## Leaf shortlist — portable today (top 40 by ported-caller count)

| VA | size | ported callers | band |
|----|-----:|---------------:|------|
| 0x5a4760 | 16 | 2 | launcher config parse + spell fx + bitmap/ZDD render + RNG |
| 0x58c8d0 | 51 | 1 | master sprite-group register + audio/music init + anim pump |
| 0x54e4f0 | 207 | 1 | cutscene dispatcher + sprite copy + tilemap collision + camera |

## Full frontier by address band

### cutscene dispatcher + sprite copy + tilemap collision + camera (1)

| VA | size | ported callers | unported deps | leaf |
|----|-----:|---------------:|--------------:|:----:|
| 0x54e4f0 | 207 | 1 | 0 | ✓ |

### master sprite-group register + audio/music init + anim pump (1)

| VA | size | ported callers | unported deps | leaf |
|----|-----:|---------------:|--------------:|:----:|
| 0x58c8d0 | 51 | 1 | 0 | ✓ |

### launcher config parse + spell fx + bitmap/ZDD render + RNG (1)

| VA | size | ported callers | unported deps | leaf |
|----|-----:|---------------:|--------------:|:----:|
| 0x5a4760 | 16 | 2 | 0 | ✓ |

