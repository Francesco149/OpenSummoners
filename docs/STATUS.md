# OpenSummoners — status at a glance

> **DERIVED FILE** — regenerate with `python3 tools/gen_port_ledger.py`.
> Headline numbers only; narrative lives in `PROGRESS.md`, "right now" in
> `HANDOFF.md`, durable RE in `findings/`, full map in `port-ledger.md`.

## Port coverage (engine-proper functions of `sotes.exe`)

```
███░░░░░░░░░░░░░░░░░  14.0% touched   (14.0% host-tested, 14.7% of code bytes)
```

| status      | count | what it means                                          |
|-------------|------:|--------------------------------------------------------|
| tested      |   216 | ported + module covered by the host unit suite       |
| ported      |     5 | reimplemented in src/, no host test for that module  |
| **touched** | **221** | tested + ported (FUN_ provenance ref in src/)    |
| unported    |  1537 | exists in engine, never referenced from src/         |

**Denominator note (read this before judging the %):** the headline % is over
**engine-proper** functions — the **1490** below
`0x5bdab0`. The other **268** non-thunk functions are the
statically-linked MSVC CRT tail (operator_new, _malloc, _strcmp, __ftol, the
`entry` startup, RtlUnwind, …); OpenSummoners is a **drop-in** that *links*
those like retail rather than porting them (PLAN.md §2-3), so counting them
would bury real progress. Full table is **1758** non-thunk
functions (of 1768 incl. thunks).

Code-byte coverage (14.7% of engine-proper bytes) is the truer progress
signal: the engine has a long tail of tiny leaf helpers, so function count
understates how much actual instruction volume is ported.

## Current front

- **Phase — the freeroam HUD panel**, built slice-by-slice bit-exact vs the errands
  freeroam real-play (`sword2.osr`). The prior arc (town intro → arrival/house/errands
  cutscene) renders 1:1; the frame-lock **sim-tick re-drive** foundation is closed
  (ckpt 168 — retail walk warmup = 1 idle tick, `CHAR_INPUT_REPEAT_DELAY` 3→2).
- **Landed (ckpt 167–174, all dhash byte-identical):** leader panel — HP/MP bars +
  numbers + frame + slide-in; element **stars**; **level** digit (+ the ramp custom-
  palette bind `bs_install_palette`/`FUN_005b7bd0`); **EXP** gauge; the 6-slot **item
  bar** (slice 2); the **door-indicator** compass algorithm (slice 3). `hud.{c,h}` +
  `game_render_hud`. Detail: `findings/freeroam-hud.md §1-9`.
- **Open blocker — the PORTRAIT.** Its bank is read live off `char+0x50` at the party
  leader-match branch, but `hud_ctx+0x1b4` (leader_uid) reads **0x0 on every call across
  every scripted replay** (Frida INT3 / field-spec / native probe all agree) — a
  **replay-fidelity gap** (the ring-injection replay doesn't arm what a live human
  session does), NOT a port bug. Resolve by finding the `+0x1b4` setter, or by probing a
  **live/manual** errands play. `findings/freeroam-hud.md §6-9`.
- **Next move:** (a) the door-indicator's `+0x1160` EFFECT-actor **spawn position source**
  (activator `0x41f200` / `game_world.c` exit-table) → retires `PORT-DEBT(hud-door-actors)`;
  (b) bottom-left "quick item" strip (`0x497c20`) → bottom-right combat cluster
  (`0x4975e0`); (c) retire `PORT-DEBT(hud-party-context)` when the party subsystem lands.
- **Open HUD PORT-DEBT:** `hud-party-context` (values + portrait), `hud-door-actors`
  (exit spawn), `hud-slide` / `hud-item-hslide` (slide ramps). See `port-debt.md`.
- **Standing bar:** every divergence is `differ_px==0` or a named/understood residual
  (`parity-ledger.md`); attribute to a pillar before suspecting logic (`parity-model.md`);
  seed-pinned both sides, compared by anchor/tick — never the flip axis.
- **Read next:** changelog → `PROGRESS.md`; deep RE → `findings/` (esp. `freeroam-hud.md`,
  `freeroam-brake-onset.md`); module layout + open threads → `memories/HANDOFF.md`
  (last rewritten ckpt 155 — stale on the HUD arc).

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
- `AGENT-WORKFLOW.md` — when to spawn subagents vs stay in the main loop.
- `PLAN.md` — goal, constraints, phased roadmap.
