# OpenSummoners — status at a glance

> **DERIVED FILE** — regenerate with `python3 tools/gen_port_ledger.py`.
> Headline numbers only; narrative lives in `PROGRESS.md`, "right now" in
> `HANDOFF.md`, durable RE in `findings/`, full map in `port-ledger.md`.

## Port coverage (engine-proper functions of `sotes.exe`)

```
███░░░░░░░░░░░░░░░░░  14.2% touched   (14.2% host-tested, 14.9% of code bytes)
```

| status      | count | what it means                                          |
|-------------|------:|--------------------------------------------------------|
| tested      |   219 | ported + module covered by the host unit suite       |
| ported      |     5 | reimplemented in src/, no host test for that module  |
| **touched** | **224** | tested + ported (FUN_ provenance ref in src/)    |
| unported    |  1534 | exists in engine, never referenced from src/         |

**Denominator note (read this before judging the %):** the headline % is over
**engine-proper** functions — the **1490** below
`0x5bdab0`. The other **268** non-thunk functions are the
statically-linked MSVC CRT tail (operator_new, _malloc, _strcmp, __ftol, the
`entry` startup, RtlUnwind, …); OpenSummoners is a **drop-in** that *links*
those like retail rather than porting them (PLAN.md §2-3), so counting them
would bury real progress. Full table is **1758** non-thunk
functions (of 1768 incl. thunks).

Code-byte coverage (14.9% of engine-proper bytes) is the truer progress
signal: the engine has a long tail of tiny leaf helpers, so function count
understates how much actual instruction volume is ported.

## Current front

- **Phase — the freeroam sim**, built bit-exact vs errands re-drives. The prior arcs
  (town intro → arrival/house/errands cutscene; the HUD panel ckpt 167–174) render 1:1.
- **Landed ckpt 175 — COLLISION:** the movers `0x54ded0`/`0x54db10` tile halves ported
  (`collision.c`) + `character_step` restructured to the retail `0x442a70` tick order
  (support probe → vertical mover/ledge-fall → ramp → worldX commit); slope ramps
  read live off the user's exe; the `LAB_00589520` "occlusion marks" = invisible
  collision WALLS (map_decode fix). **Arche stops/climbs/descends the errands stairs
  tick-exact** (dwx==dwy==0 through the whole climb; both walls flush) — retires the
  studio-note-2441 divergence + debts `char-collision-mover`/`collision-slopes`/
  `decode-occlusion-mark`. Sole residual: the 4-tick standing TURN-AROUND
  (`char-turn-state`). RE: `findings/freeroam-collision.md`.
- **HUD blocker (parked) — the PORTRAIT.** `hud_ctx+0x1b4` (leader_uid) reads 0x0 on
  every scripted replay (a replay-fidelity gap, not a port bug); resolve via the
  `+0x1b4` setter or a live/manual play. `findings/freeroam-hud.md §6-9`.
- **Next move:** (a) `char-turn-state` — the 4-tick standing turn-around
  (`0x426f50(body,2)` case-2 sub-FSM, `0x442a70:810-830`), the last freeroam-walk
  residual; (b) HUD: door-indicator spawn source / bottom strips (`0x497c20`/`0x4975e0`);
  (c) `mover-actor-scan` when collidable actors matter (combat/platforms).
- **Open PORT-DEBT (this front):** `char-turn-state`, `mover-actor-scan`,
  `char-drop-through`, `char-reverse-decel`; HUD: `hud-party-context`,
  `hud-door-actors`, `hud-slide`/`hud-item-hslide`. See `port-debt.md`.
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
