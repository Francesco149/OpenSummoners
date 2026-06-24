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
| **replay applied 1 sim-tick LATE** (the walk "−4px ramp gap") | **tooling — FIXED (ckpt 163e)** | NOT physics.  `feed_input` read `g_sim_tick_count` BEFORE `game_camera_step` bumps it (after `freeroam_step`), so a recording-tick-T trace entry drove the port body LABELLED T+1.  Fix: `feed_input` anticipates the pending increment (`sim = g_sim_tick_count + (g_game_active && even g_game_camera_hold parity)`).  General — also corrects the Z draw / dialogue X-advance timing.  VERIFIED: the walk now starts at tick 1888 == retail (held-edge 1886 + the 2-idle-tick warmup, DELAY=3, which is retail-EXACT), 0px at every settled tick (the residual −2/−3px is the recording's ~2.23-flip/tick aliasing, not a port delta — the port reproduces retail's smooth ramp; the recording stair-steps it). |
| dialogue advances on X (not just V/Enter id 0x24) | port debt | the port's dialogue/menu must consume the X→0x24 confirm; the USER spams X to advance.  Until then the converter maps X→0x24 (faithful: it IS the confirm). |
| the title menu CONFIRM isn't recorded | recorder gap (`engine_input.h`) | the `OSS_INPUT_RECORD` held-set misses the menu start confirm (only the arrows show in the title phase); the fixed BOOT nav stands in.  Fix: capture it so the title is input-driven too. |
| sword DRAW startup latency (~3-4 ticks) | **port debt — FIXED (ckpt 163f)** | retail: Z-press tick 1807 → res-0x571 fr96 at tick 1810 (+3t); the port swapped the sword-out bank + drew fr96 at press+0 (FRAMEBEG 1806, 4t early).  RE'd the Z→draw beat: Z queues a context-action (cmd[5] type 0xd2) the per-form action FSM (`442a70`→`0x45a300`, a 14 KB action-exec SM, UNPORTED) executes after a ~3t startup, THEN re-installs the form (`41f200`: 0xc35a↔0xc35b = res 0x570↔0x571).  Fix: `character_resolve_sword` defers the `sword_out` toggle by `CHAR_SWORD_DRAW_STARTUP`=4 ticks (the bank swap + the clip edge both key off it, so Arche holds sword-in idle through the startup = retail).  VERIFIED off port-sync3.osr vs sword2.osr: DRAW fr96 at **1810**, SHEATHE fr96 at **3197** (both == retail), every cel dst byte-identical, no body regression.  `PORT-DEBT(sword-draw-startup)` (the timer stands in for the unported 45a300 FSM); quirk #118; `findings/sword-draw-startup.md`. |
| body divergence at sword2 tick 2082 (−9px) + the dashes (~+20/+35/+58px) | **DIAGNOSED (ckpt 163g) — accel-phase offset hidden by camera-follow; needs a STATE-FUL recording** | NOT a steady-state physics bug: the dash CAP (`vel=−48000` = −4.8px/tick) and the brake (−800/tick) both **MATCH retail** measured at camera-CLAMPED ticks (`port-sync3` wx-vel vs the recording's screen-x deltas, camera matched dCAM≈0).  The residual screen-x gap is a small **wx-offset accumulated during the ACCEL ramp** (walk/dash start) — which happens at `wx>30000` where the camera FOLLOWS (both pinned at screen 270), HIDING it — then revealed when `wx<30000` clamps the camera.  Un-rootcauseable vs sword2.osr (no velocity state + the camera hides the accel).  See "## Chase #3 diagnosis". |
| errands-dialogue cadence (~13t residual, ckpt 145) | port debt (cutscene timing) | shows up as a small offset between the input-driven cutscene and retail. |

## Chase #3 diagnosis (ckpt 163g) — the movement residuals are camera-hidden accel-phase offsets
After chase #2 (the sword draw), the next divergences (`sync_diff port-sync3 vs sword2`) are
the brake-stop at ~2085 (−9px) and the dashes at ~2385/2655 (+20/+35/+58px).  Decisive finding
(input + port `--osr-state` wx/vel + the recording's per-tick body screen-x):
- **Steady-state physics MATCH.**  At camera-CLAMPED ticks (screen_x≠270, where the camera at the
  left edge makes screen_x track wx and dCAM≈0), the port's dash cap (`vel=−48000`) and decel
  (−800/tick) equal the recording's screen-x velocity tick-for-tick.  The walk cap (−24000) /
  brake (−800) likewise.  These were already ground-truthed live (ckpt 114/150/153) and re-confirm.
- **The residual is an ACCEL-phase wx-offset, accumulated where the camera HIDES it.**  The walk/
  dash ACCEL ramps run at `wx>30000`, where the follow-camera pins Arche at screen ~270 on BOTH
  sides — so the accumulating wx difference is invisible.  When `wx<30000` the camera clamps to the
  left edge and screen_x = wx, REVEALING the offset (constant through the parallel brake at 2085;
  ~+20px at the dash extreme 2450).  The port's smooth per-sim-tick accel vs the recording's
  ~2.23-flip/tick sampling (+ any 1-tick warmup delta) accumulates ~6-20px over an accel ramp.
- **Un-rootcauseable against THIS recording.**  sword2.osr has **no OSR_STATE** (no retail wx/vel),
  and the accel phases are camera-hidden, so there is no observable retail trajectory to diff the
  accel against.  Forcing the port's screen_x to match the recording's clamped samples would be
  curve-fitting against the recording's flip-aliasing + an unobservable accel — FORBIDDEN.
- **Unblock = a STATE-FUL retail recording.**  Re-record the errands freeroam under the proxy with
  **`OSS_OSR_STATE=1`** (the proxy's `eh_flip_cb` emits retail's player wx/vel per flip, mirroring
  the port's `--osr-state`); then `sync_diff` (or a wx/vel variant) compares the ACCEL ramps
  directly — wx vs wx, vel vs vel — independent of the camera.  Any real accel divergence then shows
  on the wx axis; if the wx ramps match, the screen_x residual is confirmed as the recording's
  flip-aliasing and the trace is frame-locked at the physics level.

## Strict bar
A trace is frame-locked when `sync_diff` reports **0px** body divergence at every SETTLED tick AND
the wx/vel ramps match a STATE-FUL recording through the accel phases (camera-independent).  Screen-x
deltas during camera-FOLLOW motion are confounded (the camera pins Arche at ~270, hiding wx) and
during camera-CLAMP motion carry the recording's ~2.23-flip/tick aliasing — neither is a curve-fit
target.  Root-cause on the wx/vel axis (decompile + the field traces), never the aliased screen_x.
