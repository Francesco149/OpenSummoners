# Freeroam walk: accel bit-exact; the brake/run "+1" is a HARNESS artifact, port CORRECT (chase #3 RESOLVED)

> **ckpt 166 UPDATE:** the harness fix is now DONE (the inject-side structural +1) and
> brake / no-warmup TRANSITION onsets re-drive BIT-EXACT.  It exposes a PROXY held-
> injection warmup residual (proxy walk-press = 1 idle tick vs real retail's 2) — **NOT a
> port bug: capdash2 turbo-off CONFIRMS the port's DELAY=3**.  Jump to "## THE HARNESS FIX
> (ckpt 166)" → "## EXPOSED by the fix".

**ckpt 165.** The frame-lock chase #3 residual (ckpt 163g hypothesised "camera-hidden
accel-phase aliasing") was chased to the camera-independent wx/hvel axis.  **Outcome
(assembly-confirmed): the WALK ACCEL is bit-exact, and the port's brake/run onset is
CORRECT — the apparent "1 tick early" is a proxy re-drive labeling artifact, NOT a port
bug.**  See "## RESOLUTION" below.  (The narrative below the next two sections records the
investigation as it unfolded — the "real bug" framing in them is SUPERSEDED by RESOLUTION.)

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

## RESOLUTION (ckpt 165, USER-directed deep RE) — the PORT IS CORRECT; the +1 is a HARNESS artifact
The "real bug" reading above was itself confounded.  The USER asked for the EXACT code
path (match retail's order of operations, not curve-fit), incl. whether DInput is in the
loop.  Tracing it — decompile + **the unpacked-exe assembly** — proves retail's
input→velocity is **single-frame, zero latency**, so the port's immediate brake is RIGHT:

- **Frame order** `0x439690`: input poll `0x468a20`@866 → scene update `0x46cd70`@1099
  (reduction→apply) → easer bump `0x43d1d0`@1123 → present.  The velocity is computed
  BEFORE the sim-tick bump; the port mirrors this (`feed_input`→`freeroam_step`→
  `game_camera_step` bump→`drive_present`).
- `0x468a20` clears+sets the held-axis from the CURRENT kbstate (producer `0x46a880`),
  then the reduction `0x478ba0` reads it and **zeroes the walk command immediately on
  release** (no command-side latch).
- **The decisive bit — the command buffer is FRESH (assembly):** the reduction
  `0x478ba0` is a thiscall with `this = the actor` (`46ce4e mov esi,ecx`; esi=actor,
  `*(actor+0x40)`=mgr is the arg), so it writes the command to **`actor+0x14854`**.  The
  apply `0x485fc0`→`0x442a70` is ALSO `this = the actor` (`46ceaa mov (esi,eax),ecx`, the
  party-slot actor) and reads **`actor+0x14854`** (`485fc0:348` → `442a70(actor+0x5215,
  ..)`).  Same actor, same buffer, phase-A reduction → phase-B apply IN ONE FRAME — the
  apply consumes the command the reduction just wrote.  **No buffer copy, no apply lag.**
- **DInput is NOT the cause:** the proxy injects at the `0x5ba520` leaf, downstream of
  DInput's GetDeviceState, yet still reproduces the +1 — so it's in the labeling, not the
  input source.

**So real retail brakes on the release tick (release+0) — exactly like the port.**  The
re-drive's brake/run **+1 is a proxy injection-labeling artifact**: the injection fires
against the PRE-bump `g_eh_sim_tick`, while `OSS_INPUT_RECORD` (and the port's
`feed_input` anticipation) label at the POST-bump emit tick — a +1 the warmup absorbs on
the press (clock-threshold latching is robust to ±1) but exposes on the no-warmup
brake/run.  The labeling-fix test (`OSS_INJECT_LEAD`/`OSS_EMIT_TICK_BIAS`, both = a
UNIFORM relabel) couldn't reconcile it precisely because the warmup interacts
non-uniformly — NOT because there is a real engine latency.  **No port fix; that would
INTRODUCE a regression.**

## THE HARNESS FIX (ckpt 166) — the structural +1; transition onsets now BIT-EXACT
The TODO below is DONE.  The faithful correction is INJECT-side, not a relabel: a
tick-T entry must land its input in the frame that EMITS as label T.  Frame loop
`0x439690` runs, once per logic frame: input poll `0x468a20`(@866) → velocity
`0x46cd70`(@1099) → easer `0x43d1d0`(@1123, `g_eh_sim_tick++`) → present `0x5b8fc0`
(FRAMEBEG labeled POST-bump).  So input read at the poll (pre-bump `g_eh_sim_tick`)
emits under label `g_eh_sim_tick + 1`.  Fire tick entries at the PENDING emit label
`g_eh_sim_tick + 1` (`engine_input.h` `EI_TICK_AXIS`), NOT at `g_eh_sim_tick` (that
lands velocity at T+1 — the +1).  This MIRRORS the port's `feed_input` anticipation
(`main.c sim = g_sim_tick_count + pending-bump`) + the recorder's flip→tick map, so
port and re-drive apply a tick-T entry under the SAME emit label, offset-0.
- **VERIFIED** (`port-move.osr` vs `retail-harnessfix.osr`, new default, `state_diff`):
  the no-warmup BRAKE onset is now **bit-exact** — retail brakes on the release tick
  **2160 == port** (was 2161, the +1).  Walk accel + steady-state stay bit-exact.
- A pure relabel (`OSS_EMIT_TICK_BIAS=-1`) ALSO joins the brake but is WRONG in kind: it
  renames the emit without moving retail's real input-read frame, so the warmup/physics
  ran un-anticipated.  Superseded; kept default 0 as a residual diagnostic.

## EXPOSED by the fix — a PROXY held-injection warmup residual (NOT a port bug)
The structural +1 is correct for the no-warmup brake (proven), but it surfaces that the
proxy's tick-axis HELD-injection produces a walk-press warmup **1 tick shorter** than
real retail.  At the faithful label both sides press at 2050; the proxy moves at **2051
(1 idle tick)** while the port `CHAR_INPUT_REPEAT_DELAY=3` models **2 idle ticks** (2052).
**The port is CORRECT — this is a HARNESS residual, not a port debt.**  Decisive table
(entry press@2050, release@2160; port == capdash2 ground truth):

| | press(motion) | brake(onset) | warmup | labeling |
|-|-|-|-|-|
| **port (== capdash2 GT)** | 2052 | 2160 | 2 idle | correct |
| proxy lead 0 | 2052 *(coincidence)* | 2161 | 1 idle, applied-1-late cancels | +1 late |
| proxy **structural +1** | 2051 | **2160** | 1 idle | **correct** |
| proxy +1, **turbo OFF** | 2051 | 2160 | 1 idle | correct |
| capdash2 (frida, turbo OFF) | t1552 (+2) | — | 2 idle | — |

- **capdash2 CONFIRMS the port.**  `capdash2` (ckpt 118, FRIDA, **turbo OFF** = the SAME
  lockstep clock, step 16 ms) shows a HELD-right walk warmup of **2 idle ticks** (raw
  `call_trace`: held@tick 1550 → hvel 1600 @tick **1552**), the bytes embedded
  "field-exact" in `test_character_run_ramp`.  So `DELAY=3` is right; **do NOT touch the
  port.**  (The earlier "port may be 1 long" read was WRONG — it predated the turbo-off +
  capdash2-raw checks.)
- **Turbo is RULED OUT.**  Re-driving with `OSS_TURBO=0` (lockstep bypasses turbo once
  armed anyway) STILL gives the proxy 1 idle tick — so the gap is not the clock.
- **ROOT = held-injection fidelity (harness-sensitive wall-clock warmup).**  Same
  `0x478ba0` code, same lockstep clock, yet frida flip-axis held = 2 idle, proxy tick-axis
  held = 1 idle.  The warmup is the `0x478ba0:182` `GetTickCount()-press_ts >= …` wall-clock
  test keyed on the edge timestamp `device+0x14c` (set by the producer `0x46a880` on the
  0→1 edge); the two injection harnesses present that edge/`press_ts` 1 tick apart.  At
  lead 0 the proxy's intrinsic 1-short warmup was HIDDEN (applied-1-late + warmup-1-short
  cancel → press matched the port); the +1 (correct for the brake) un-hides it on the
  rising edge.  **No curve-fit "delay held rising edges by 1"** until `0x46a880`'s edge/
  `press_ts` capture is RE'd and the frida-vs-proxy 1-tick difference is understood.

## Status
- Walk accel + steady-state: **regression-locked bit-exact** (`parity-ledger`).
- Brake / no-warmup TRANSITION onsets: **HARNESS FIX LANDED (ckpt 166) — bit-exact** (the
  structural +1; brake-verified).  This is the USER's stated goal for the harness fix.
- Walk-press (warmup-anchored rising edge): a known **+1 PROXY residual** — the held-
  injection under-warms by 1 vs real retail.  Port is CORRECT (capdash2-confirmed).  For
  warmup-anchored onset timing use the RECORDING / the port's `feed_input` replay, not the
  live held re-drive.  Deeper fix = RE `0x46a880` edge/`press_ts` (OPEN).
- TRADE-OFF NOTE: the +1 trades the lead-0 coincidental press match for a faithful brake.
  Kept because it makes the LABELING principled (== port feed_input + recorder) and fixes
  the transition onsets the USER asked for; the press residual is narrow + documented.
- Dash: the synth double-tap (held-axis only) capped at the WALK cap (24000) — the real
  dash (cap 48000) needs explicit RING double-tap edges (`ids:[4,4]`, ckpt 154); the
  retail dash accel (two-phase +3200→+1600 to 48000) was captured + is bit-exact vs the
  port (modulo the same +1 harness onset).
