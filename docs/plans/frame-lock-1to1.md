# Frame-lock 1:1 — driving both sides frame-for-frame (the porting foundation)

> **USER directive (ckpt 163d):** the basis for porting new things is being able to drive
> the PORT and RETAIL 1:1 frame-by-frame off a real-play recording.  Any divergence is
> **previous port debt or a tooling gap** — never a "good enough" approximation.  The
> system must be **general** (any recorded trace), not ad-hoc for one capture.

## The loop

1. **RECORD** (retail, under the capture proxy): a real-play session yields
   - `<name>.osr` — the draw stream (`tools/capture_proxy/run_proxy.sh`), and
   - `<name>-input.jsonl` — the per-frame held-key set (`OSS_INPUT_RECORD`, the leaf-query
     hook `0x5ba520`; tracks arrows + Z/X/C + Enter/V — `engine_input.h`).
2. **CONVERT** — `tools/trace_studio2/sync_inputs.py <rec.osr> <rec-input.jsonl> [offset]`
   turns the flip-recorded inputs into port replay traces (`runs/sync/<name>-{held,nav}.jsonl`):
   - a **fixed BOOT nav** (frame-axis) — title → new game → `enter_game` (the recorder does
     NOT capture the title menu confirm → the one not-yet-input-derived part, a recorder gap);
   - the **in-game inputs** on the **sim-tick axis** (deterministic; the port's flip rate
     differs from retail's): the held LEVELS → the held-trace, the press EDGES → ring ids
     (Z→9, dirs→1-4, **X/Enter/V/ESC→0x24 confirm** — the USER advances the dialogue + menus
     by spamming X; the freeroam reads the held X for the attack).
   - **offset 0** by construction: the in-game sim-tick starts at `enter_game` on BOTH sides,
     so the recording's in-game ticks align with the port's directly.  A residual offset ⇒ the
     port's cutscene cadence diverges from retail's (a gap to close, not to tune away).
3. **REPLAY** — `opensummoners.exe --osr-emit <port>.osr --osr-state --input-trace
   <name>-nav.jsonl --held-trace <name>-held.jsonl` (inside `nix develop`).  During a replayed
   freeroam the **deterministic input clock** (`input_now()`, `main.c`) pins `GetTickCount`
   to `sim_tick * 33 ms` so every wall-clock-keyed check (the dash double-tap window, the
   attack refractory, the ring-record timestamps, the 100 ms consume) replays deterministically.
4. **DIFF** — `tools/trace_studio2/sync_diff.py <port>.osr <rec>.osr <offset> [lo hi tol]`
   reports Arche's body screen-x + a camera-proxy tile-x per sim-tick, attributing each
   divergence to MOVEMENT (body differs, camera same) vs CAMERA, and flags the FIRST one.
   Visual side-by-side: the `osr_view` studio (tick-joined; the offset baked into the shortcut).

## Known gaps (the foundation surfaces these — each is port debt / a tooling gap)

| gap | kind | status |
|-----|------|--------|
| dialogue advances on X (not just V/Enter id 0x24) | port debt | the port's dialogue/menu must consume the X→0x24 confirm; the USER spams X to advance.  Until then the converter maps X→0x24 (faithful: it IS the confirm). |
| the title menu CONFIRM isn't recorded | recorder gap (`engine_input.h`) | the `OSS_INPUT_RECORD` held-set misses the menu start confirm (only the arrows show in the title phase); the fixed BOOT nav stands in.  Fix: capture it so the title is input-driven too. |
| walk-accel ramp — first body divergence (sword2 tick 1898, −4px, re-converges) | port debt (movement physics) | the port lags ~4px during the walk's acceleration. |
| dash DISTANCE (~20px after the first dash) | port debt (movement) | the dash now FIRES (deterministic clock) but covers ~20px less — start/decel precision. |
| errands-dialogue cadence (~13t residual, ckpt 145) | port debt (cutscene timing) | shows up as a small offset between the input-driven cutscene and retail. |

## Strict bar
A trace is frame-locked when `sync_diff` reports **0px** body divergence across the whole
in-game span (camera too).  Anything else is a defect to root-cause — read the decompile +
the field/flow traces, fix the logic, re-diff.  No curve-fit, no approximation.
