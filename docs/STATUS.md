# OpenSummoners — status at a glance

> **DERIVED FILE** — regenerate with `python3 tools/gen_port_ledger.py`.
> Headline numbers only; narrative lives in `PROGRESS.md`, "right now" in
> `HANDOFF.md`, durable RE in `findings/`, full map in `port-ledger.md`.

## Port coverage (engine-proper functions of `sotes.exe`)

```
██░░░░░░░░░░░░░░░░░░  11.8% touched   (11.8% host-tested, 13.1% of code bytes)
```

| status      | count | what it means                                          |
|-------------|------:|--------------------------------------------------------|
| tested      |   184 | ported + module covered by the host unit suite       |
| ported      |     5 | reimplemented in src/, no host test for that module  |
| **touched** | **189** | tested + ported (FUN_ provenance ref in src/)    |
| unported    |  1569 | exists in engine, never referenced from src/         |

**Denominator note (read this before judging the %):** the headline % is over
**engine-proper** functions — the **1490** below
`0x5bdab0`. The other **268** non-thunk functions are the
statically-linked MSVC CRT tail (operator_new, _malloc, _strcmp, __ftol, the
`entry` startup, RtlUnwind, …); OpenSummoners is a **drop-in** that *links*
those like retail rather than porting them (PLAN.md §2-3), so counting them
would bury real progress. Full table is **1758** non-thunk
functions (of 1768 incl. thunks).

Code-byte coverage (13.1% of engine-proper bytes) is the truer progress
signal: the engine has a long tail of tiny leaf helpers, so function count
understates how much actual instruction volume is ported.

## Current front

- **Phase:** Phase 4–5 — porting the **in-game town backdrop** render path (the first
  in-game visible frame), building toward a trace that plays 1:1 pixel-perfect frame by
  frame on both sides. Milestone map: `ROADMAP.md`. Mechanical next chip:
  `port-frontier.md`.
- **Where we are (ckpt 61):** the **decode → grid → geometry → draw-list** chain is
  CLOSED. `map_decode` places tiles in the runtime render grid (`map_grid`),
  `map_render_tile` reads the geometry, and `map_render_walk` + `draw_pool` (the
  27-layer draw-node pool, `0x4917b0`/`0x586010`) accumulate the per-frame draw-node
  list. All pure + host-tested: **806 pass / 0 fail / 6 skip**. Ledger **189/1490
  touched / 184 tested**. Both GUI builds clean; the new modules are in the `src`
  wildcard but **not yet called by `main.c`**.
- **Next move:** the **present pass** — walk the 27 layers, resolve each node's sprite,
  zdd-blit it (the consumer that turns the draw list into pixels) — OR wire
  `map_decode` + `map_render_walk` + present into `main.c`'s in-game scene and diff vs
  `runs/tas-ingame-1` anchored on `game_enter`. The camera/view object construction
  stays a rock (`cam[0x34..0x74]` are updated dynamically by gameplay scroll across many
  functions — no clean pure init); host tests use synthetic cameras (window math exact).
  Full writeup: `findings/in-game-intro.md`.
- **Tooling front:** rigor scaffolding just landed (this file + `CLAUDE.md` +
  `parity-model.md` + `port-debt.md` + the ods cross-ref/proof framework). The tracked
  next tooling phase is the unified `scenario-test.py` + a field-bearing flow trace
  (`docs/plans/`).
- **Standing bar:** every divergence is `differ_px == 0` or a named/understood residual
  (`parity-ledger.md`); attribute to a pillar before suspecting logic
  (`parity-model.md`); seed-pinned both sides, compared by anchor/tick.

> Hand-edited in `docs/FRONT.md`, injected here verbatim so it can't drift.

## Where to read next

- `STATUS.md` (this file) — 60-second orientation.
- `../CLAUDE.md` — the dense auto-loaded entry point (conventions, parity model, paths).
- `FRONT.md` — the hand-edited current front (source of the block above).
- `HANDOFF.md` — rolling current-checkpoint detail (module layout + open threads).
- `parity-model.md` — the multi-pillar model; `parity-ledger.md` — confirmed-1:1 guard.
- `port-debt.md` — synthetic/MVP shortcuts owed back.
- `port-ledger.md` / `.json` — per-function port status (derived).
- `port-frontier.md` — what to port next: unported fns reachable from ported
  code, zero-dep leaves ranked (derived; `tools/gen_frontier.py`).
- `ROADMAP.md` — milestones + subsystem map with difficulty / target module.
- `PROGRESS.md` — dated narrative changelog.
- `findings/INDEX.md` — map of subsystem RE writeups.
- `findings/engine-quirks.md` — the running quirk log (cite in commits).
- `AGENT-WORKFLOW.md` — how to work on this repo (read at session start).
- `PLAN.md` — goal, constraints, phased roadmap.
