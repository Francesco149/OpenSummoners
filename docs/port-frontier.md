# OpenSummoners — port frontier (what to port next)

> **DERIVED FILE** — `python3 tools/gen_port_ledger.py && python3 tools/gen_frontier.py`.

The unported, non-thunk, engine-proper functions already **called by ported code** — the natural next chips. A **leaf** has all of its
own engine callees ported, so it can land today with zero new dependencies. Sorted by how many ported callers want it.

For the *forward* port path (the title-menu scene runner and what it calls) and the semantic milestone order, see `ROADMAP.md` — some of that path isn't yet reachable from ported code so won't appear here.

- frontier functions: **0**
- of those, zero-dependency **leaves: 0** (recommended order below)

## Leaf shortlist — portable today (top 40 by ported-caller count)

| VA | size | ported callers | band |
|----|-----:|---------------:|------|
| — | — | — | (none yet — touched set too small) |

## Full frontier by address band

