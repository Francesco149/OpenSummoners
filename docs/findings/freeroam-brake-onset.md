# Freeroam walk: accel bit-exact, BRAKE onset 1 tick early (chase #3 RESOLVED)

**ckpt 165.** The frame-lock chase #3 residual (ckpt 163g hypothesised "camera-hidden
accel-phase aliasing") is a **REAL port bug**, root-caused on the camera-independent
wx/hvel axis: the port's **brake (release→decel) starts 1 sim-tick too early**.

## How it was caught (the re-drive that ckpt 164 unblocked)
The proxy injection foundation (ckpt 164) + a **sim-tick-axis injection** (this ckpt,
`engine_input.h`: `{"tick":N}` entries fire on `g_eh_sim_tick`, `{"frame":N}` on the
flip counter) let retail be driven by the SAME sim-tick trace the port replays, so
port↔retail align offset-0.  Recipe (`runs/sync/`, gitignored — regenerable):
- **retail:** `run_proxy.sh 30 spam-confirm-nav.jsonl synth-move2.jsonl` with
  `OSS_OSR_STATE=1` → `retail-move.osr` (wx/hvel per tick).
  - `spam-confirm-nav` = the proven frame boot prefix (640-1005) + DENSE tick confirms
    (id 36 every 14t, 660-1980).  **The brute-force confirm spam is what finally clears
    the ckpt-157 errands HANDOFF** — the precise `nav-full-errands` confirms missed the
    errands tutorial when it loaded late (tick 1803), leaving control locked; spamming
    advances every prompt regardless of cadence.  Arche then becomes controllable.
  - `synth-move2` = a tick-axis HELD movement (right hold @2050, release @2160, …) —
    arrows only (the held-axis path drives movement; it does NOT derive discrete rings,
    so dialogue confirms MUST be explicit nav rings — the recorded held trace alone
    freezes at the arrival dialogue, wx=41600).
- **port:** `run-opensummoners.sh --osr-emit … --osr-state --input-trace
  spam-confirm-nav --held-trace synth-move2` → `port-move.osr`.
- **diff:** `tools/trace_studio2/state_diff.py port-move.osr retail-move.osr <lo> <hi>`
  (wx relative to each side's walk-start, hvel absolute; both normalised from
  `wx`/`hvel` ↔ `fr_wx`/`fr_vel`).

## The result (synth-move2: hold right @2050, release @2160)
- **WALK ACCEL — BIT-EXACT** (14 ticks, 0 divergence): both start motion at tick **2052**
  (2-idle-tick warmup, retail-exact), hvel `1600,3200,…,24000` (accel +1600/tick, cap
  24000) identical, relwx identical every tick.
- **BRAKE ONSET — 1 TICK EARLY in the port:**
  | tick | 2159 | 2160 (release) | 2161 | 2162 | … |
  |------|------|------|------|------|---|
  | retail hvel | 24000 | **24000** | 23200 | 22400 | −800/t |
  | port hvel   | 24000 | **23200** | 22400 | 21600 | −800/t |
  Retail holds full velocity on the release tick and brakes the NEXT tick; the port
  brakes ON the release tick.  Constant **−240 wx (−2.4px)** lag after the brake (relwx
  4440 vs 4680).  This is the "brake-stop residual" — a real divergence, not aliasing.

## Root cause (port side, confirmed)
`character_step` (`src/character.c:75-81`): a held PRESS latches `cmd_dir` only after
`CHAR_INPUT_REPEAT_DELAY` warmup ticks, but a RELEASE sets `cmd_dir = 0` **immediately**
(`want==0 → cmd_dir=0`), so the brake (the else-branch `ramp_toward(vel,0,BRAKE)`) runs
the same tick.  Retail brakes one tick later — a **1-tick input→command latency** that
the port's asymmetric warmup (press-only) does not reproduce.

The single coherent model: retail has a **1-tick input→integrator pipeline** + a 1-tick
accel windup.  start = pipeline(1)+windup(1) = 2 (matches); brake = pipeline(1)+0 = 1
(the port's pipeline(0)+windup(2)=2 matches start but pipeline(0)+0=0 mis-fires the
brake).  **FIX (candidate, pending decompile confirmation):** `character_step` reads the
PREVIOUS tick's `axis_held` (1-tick pipeline) + `CHAR_INPUT_REPEAT_DELAY` 3→2, so start
stays 2052 and brake moves to 2161.  MUST re-verify no regression on the
ckpt-159/160/161/162 dash/attack/jump timings (all tuned to the current path).  RE the
retail frame order (does `0x478ba0` read the current or previous held axis vs the
`0x46a880` producer?) before shipping — don't curve-fit the +1.

## Status
- Walk accel: **regression-locked bit-exact** (`parity-ledger` candidate).
- Brake onset: **OPEN** — the fix above pending RE + no-regression re-verify.
- Dash: the synth double-tap (held-axis only) capped at the WALK cap (24000) — the real
  dash (cap 48000) needs explicit RING double-tap edges (id 4 ×2); not yet captured.
