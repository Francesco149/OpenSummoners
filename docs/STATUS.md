# OpenSummoners — status at a glance

> **DERIVED FILE** — regenerate with `python3 tools/gen_port_ledger.py`.
> Headline numbers only; narrative lives in `PROGRESS.md`, "right now" in
> `HANDOFF.md`, durable RE in `findings/`, full map in `port-ledger.md`.

## Port coverage (engine-proper functions of `sotes.exe`)

```
███░░░░░░░░░░░░░░░░░  14.4% touched   (14.4% host-tested, 14.9% of code bytes)
```

| status      | count | what it means                                          |
|-------------|------:|--------------------------------------------------------|
| tested      |   222 | ported + module covered by the host unit suite       |
| ported      |     5 | reimplemented in src/, no host test for that module  |
| **touched** | **227** | tested + ported (FUN_ provenance ref in src/)    |
| unported    |  1531 | exists in engine, never referenced from src/         |

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
  `decode-occlusion-mark`. The 4-tick standing TURN-AROUND (`char-turn-state`) is
  now PORTED (ckpt 177 below). RE: `findings/freeroam-collision.md`.
- **HUD blocker (parked) — the PORTRAIT.** `hud_ctx+0x1b4` (leader_uid) reads 0x0 on
  every scripted replay (a replay-fidelity gap, not a port bug); resolve via the
  `+0x1b4` setter or a live/manual play. `findings/freeroam-hud.md §6-9`.
- **Landed ckpt 176 — dialogue mark t1197 FIXED + char-turn RE corrected.** (a1) The
  "port skips dialogue early" mark is NOT an advance-gate bug — the advance PRESSES
  already match retail (port DLGT + dialogue_timeline off the stairs pair).  It was a
  box RENDER linger: retail keeps the arrival L9 box up through tick 1200, the port
  cleared it at 1192 → `ARRIVAL_EXIT_BOX_HOLD=8` (== the house-exit box-hold pattern),
  drawcall-verified L9 adv 1192→1200 == retail.  `findings/dialogue-advance-early.md`.
  (c) char-turn RE CORRECTED: the ckpt-175 "case-2" pointer was WRONG (that's the
  DOWN/crouch, already ported); the real reversal turn is the STATE-1 horizontal FSM
  `0x442a70:1011-1090`.  See `port-debt.md`.
- **Landed ckpt 177 — `char-turn-state` PORTED (retires the collision residual).**
  The from-rest REVERSAL now plays retail's 8-tick pivot: `CHAR_TURN_HOLD=4`
  STATIONARY windup ticks (facing HELD, the fr-6 turn cel) → flip facing + walk ramp
  (fr-7 → +152 fr-159 lingers 4 render ticks) → walk (fr 160).  `character.c`
  `turn_ctr`/`turn_frame` + `ARCHE_TURN_CLIP` {6,7} (both dirs via the +152 mirror,
  == the house turn 158→7).  GROUND TRUTH retail-stairs res 0x570 (fr 6×4 → 159×4 →
  160); `draw_probe port-stairs2` == retail, `state_diff` RIGHT 0-div/301t (no regress)
  + LEFT ramp-shape bit-exact (the −960 gap GONE).  RESIDUAL: a constant 1-tick
  reversal-ONSET phase (−240) — the port latches the reverse press 1t before retail's
  warmup gate (→ `char-input-autorepeat`, not the turn), within retail's ±1-2t
  coalescing slop.  RE: `findings/freeroam-turn-around.md`.  **USER: verify the pivot
  animation — studio shortcut = `port-stairs2 | retail-stairs` @ tick ~2950.**
- **Next move (the residual dialogue drift + the 2nd USER mark):**
  (a2) **the chain still plays EARLY (OPEN)** — a BEAT-duration gap, NOT the dialogue
  gate: under the dense-confirm re-drive the house starts ~−10t, errands ~−26t (the
  advances match, but the room-transition FADE beats settle short).  Suspects: the
  scene_fade grid settle-rate + the exit/entry WAITs (measured stand-ins off the OLD
  retail.osr).  RE the fade PERFORMER's per-tick step off retail-stairs before
  adjusting — do NOT curve-fit.  `findings/dialogue-advance-early.md` "Component 2/3".
  (b) **missing house props** (mark t2278: the stove's steaming COOKING POT + the
  kitchen HUTCH with dishes, upstairs) — the unported object-spawn PLACEHOLDER pass
  (`0x58c8c0` is a 4-B getter; the real spawn family is `0x58c8d0`/`0x58cb30`; the
  res_explorer already renders these host-side).  **CAVEAT (ckpt 176): the t2278 raw
  differ (22498) was CONFOUNDED by the char-turn (c) offset — NOW LARGELY RESOLVED
  (ckpt 177): (c) is ported, so the LEFT-walk shift drops from ~960 to ~240 wx (1 tick);
  the port DOES render most of the scene (res 1071/1072/1082/1722/1026 all present at
  t2278).  Re-capture + assess (b) at t2278 (or a PRE-reversal tick) to isolate the
  truly-missing placeholder objects.**  Mom's pose in-crop differs too — check her clip
  once the props land.  (Visual-verify: deferred.)
  (c) `char-turn-state` — **DONE ckpt 177** (`findings/freeroam-turn-around.md`).
  (d) HUD: door-indicator spawn source / bottom strips; `mover-actor-scan` when
  collidable actors matter.
- **Open PORT-DEBT (this front):** `mover-actor-scan`,
  `char-drop-through`, `char-reverse-decel`, `char-input-autorepeat` (the residual
  1-tick reversal onset); HUD: `hud-party-context`,
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
