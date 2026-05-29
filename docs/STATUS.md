# OpenSummoners — status at a glance

> **DERIVED FILE** — regenerate with `python3 tools/gen_port_ledger.py`.
> Headline numbers only; narrative lives in `PROGRESS.md`, "right now" in
> `HANDOFF.md`, durable RE in `findings/`, full map in `port-ledger.md`.

## Port coverage (engine-proper functions of `sotes.exe`)

```
█░░░░░░░░░░░░░░░░░░░  7.2% touched   (7.2% host-tested, 9.6% of code bytes)
```

| status      | count | what it means                                          |
|-------------|------:|--------------------------------------------------------|
| tested      |   114 | ported + module covered by the host unit suite       |
| ported      |     3 | reimplemented in src/, no host test for that module  |
| **touched** | **117** | tested + ported (FUN_ provenance ref in src/)    |
| unported    |  1641 | exists in engine, never referenced from src/         |

**Denominator note (read this before judging the %):** the headline % is over
**engine-proper** functions — the **1490** below
`0x5bdab0`. The other **268** non-thunk functions are the
statically-linked MSVC CRT tail (operator_new, _malloc, _strcmp, __ftol, the
`entry` startup, RtlUnwind, …); OpenSummoners is a **drop-in** that *links*
those like retail rather than porting them (PLAN.md §2-3), so counting them
would bury real progress. Full table is **1758** non-thunk
functions (of 1768 incl. thunks).

Code-byte coverage (9.6% of engine-proper bytes) is the truer progress
signal: the engine has a long tail of tiny leaf helpers, so function count
understates how much actual instruction volume is ported.

## Current front

- **Phase:** Phase 1 surface-map complete; Phase 4 in progress on the title path. All small leaves the title-menu scene runner needs are ported (pixel drawer, asset register, bitmap session, ZDD wrapper, cs_dispatch, WndProc, app_pump). Next front: the title-menu scene runner itself (FUN_0056aea0, 3441 B) — first visible frame. See ROADMAP.md, HANDOFF.md.
- **Top blocker:** FUN_0056aea0 is a multi-checkpoint port: outer skeleton + state-vars + the PTR_DAT_0056bfa4 7-handler jumptable, plus unported helpers (FUN_00412c10 menu-controller alloc, FUN_0043c110/_43ce50 input poll/latch, FUN_0056c070/_0056c180 sparkle+flip). See findings/title-scene.md.

## Where to read next

- `STATUS.md` (this file) — 60-second orientation.
- `HANDOFF.md` — "where to pick up *right now*" (rewritten each checkpoint).
- `port-ledger.md` / `.json` — per-function port status (derived).
- `port-frontier.md` — what to port next: unported fns reachable from ported
  code, zero-dep leaves ranked (derived; `tools/gen_frontier.py`).
- `ROADMAP.md` — milestones + subsystem map with difficulty / target module.
- `PROGRESS.md` — dated narrative changelog.
- `findings/INDEX.md` — map of subsystem RE writeups.
- `findings/engine-quirks.md` — the running quirk log (cite in commits).
- `AGENT-WORKFLOW.md` — how to work on this repo (read at session start).
- `PLAN.md` — goal, constraints, phased roadmap.
