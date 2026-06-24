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
- **Unblock = RE-DRIVE retail with the recorded input + state (USER-suggested, ckpt 163g; no new
  play-through needed).**  The proxy ALREADY replays input (`OSS_HELD_TRACE` = the held-scancode
  trace, `OSS_INPUT_TRACE` = rings; the engine DERIVES the ring edges from the held transitions on
  replay, `engine_input.h`) and gates a state pass on `g_cfg.state_on` (currently emits only `rng`,
  `engine_hooks.h eh_flip_cb`).  So: re-drive retail under `run_proxy.sh` with `OSS_HELD_TRACE`=the
  recorded trace + the state pass on, capturing retail's per-flip wx/hvel — the SAME deterministic
  RNG-free movement the port runs, now with state.  Then a wx/vel diff (port `--osr-state` vs the
  retail re-drive) compares the ACCEL ramps directly — wx-vs-wx — independent of the camera.

## Chase #3 execution plan (the re-drive — teed up, ckpt 163g)
1. **Extend the proxy state pass** (`tools/capture_proxy/engine_hooks.h eh_flip_cb`, after the `rng`
   field, under `g_cfg.state_on`): emit the player **wx** + **hvel** off the KNOWN leader chain
   (`tools/flow/freeroam_mover_fields.json`): `room_state = *(0x8a9b50)+0x2784 -> +0x200c leader-slot
   -> +0x9f4 entity -> +0x40 body`, then **wx = body+4**, **hvel = body+0x28** (NB the field-spec's
   "+0x18 vel" is the VERTICAL accumulator; the HORIZONTAL one the port's `fr_vel` mirrors is +0x28).
   VirtualQuery-guard each deref (the chain is null pre-freeroam).  Optionally also `facing` (+0x2c).
2. **Forward the env in `run_proxy.sh`** (the passthrough list ~line 94): add `OSS_HELD_TRACE`
   `OSS_INPUT_TRACE` + whatever toggles `g_cfg.state_on` (mirror the port's `--osr-state`).
3. **Re-drive:** `run_proxy.sh` with `OSS_HELD_TRACE` = the recorded held trace (raw flip-axis
   `sword2-input.jsonl`, OR the converted held trace) + seed-pin + state on -> `retail-state.osr`
   carrying retail's wx/hvel per flip (sim_tick on each FRAMEBEG for the join).
4. **Diff wx-vs-wx:** a `sync_diff` variant (or `osr.py STATES`) aligns port `--osr-state` wx/vel vs
   the retail re-drive's, per sim-tick, through the walk/dash ACCEL ramps (no camera, no screen_x).
5. **Root-cause + fix:** if the wx ramps diverge, RE the accel/warmup in `442a70` (the integrator) /
   `character_resolve_run`/`_step` and fix the logic; if they MATCH, the screen_x residual is
   confirmed flip-aliasing and the trace is frame-locked at the physics level (document + close).

Steps 1-2 are DONE + committed (`3ea94e8`: eh_flip_cb emits wx/hvel/facing; run_proxy forwards
OSS_OSR_STATE).  Step 3 is BLOCKED — see below.

## Chase #3 BLOCKER — RESOLVED (ckpt 164) — the proxy now INJECTS instead of shadowing ddraw
**FIXED.** Root cause: the game loads `System32\ddraw.dll`, not the app-dir `ddraw.dll` drop, so
the old "drop ddraw.dll, the exe auto-loads it" scheme never loaded our proxy (empty logs all
session).  FIX = `build/inject.exe` (`inject.c`): `CreateProcess(SUSPENDED)` the unpacked exe →
remote `LoadLibraryA(<full path to oss_proxy.dll>)` → `ResumeThread`.  Our DLL loads regardless
of the ddraw search order AND is live before the main thread, so the engine-VA hooks patch the
mapped sotes code and the harness dismisses the `#32770` launcher IN-PROCESS (hands-free, no
click).  `run_proxy.sh` rewritten to use it.  Committed `1a31089`.
VERIFIED: a no-input run captures 2.3GB + emits the M8 STATE fields (wx hvel facing rng); a
boot-ring-nav run (`nav-full-errands.jsonl` as `OSS_INPUT_TRACE`) reaches **newgame_enter 692 /
prologue_enter 825 / game_enter 1117** + the RNG re-pin — retail gameplay is driven again.
**NEXT (the actual chase-#3 re-drive, now unblocked):** drive retail to the ERRANDS freeroam +
inject MOVEMENT (walk + dash) so wx/hvel exercise the accel ramps, capture with `OSS_OSR_STATE=1`
→ `retail-state.osr`; drive the PORT the same way (`--osr-state`); then the wx-vs-wx diff (step 4).
The held trace alone keeps retail at the TITLE (the title-confirm recorder-gap — `sword2-input.jsonl`
has no title confirm); use the boot ring nav (`runs/sync/sword2-nav.jsonl` boot prefix /
`nav-full-errands.jsonl`) for boot→game_enter, then a movement trace timed to the errands handoff.

## (historical) Chase #3 BLOCKER (ckpt 164) — the native proxy wasn't loading into retail
Every `run_proxy.sh` retail capture this session came back with an EMPTY proxy log + no `.osr`,
EVEN when the launcher was dismissed (`[launcher] dismissed` logged, exe left locked = game ran).
So the game ran but our `ddraw.dll` proxy never engaged.  Diagnosis (ckpt 164):
- **Our DLL is FUNCTIONAL.**  A full-path `LoadLibrary(<game-dir>\ddraw.dll)` from a 32-bit test
  loader runs our `DllMain` → `[proxy] DLL_PROCESS_ATTACH` + `[cfg]` log + IAT/clock hooks (only
  `engine_hooks_install` fails, expected — the loader isn't sotes.exe at 0x400000).  So `proxy_init`
  is fine; the DLL isn't broken.
- **A BARE `LoadLibrary("ddraw.dll")` from the game dir resolves to `C:\WINDOWS\SYSTEM32\ddraw.dll`,
  NOT our drop** (`GetModuleFileName` confirmed).  So the game appears to load the SYSTEM ddraw and
  never our proxy.  NOT KnownDLLs (37 entries, ddraw absent).  **DDRAW is a STATIC import** (no Delay
  Import Tables in the exe), so the loader resolves it at process start — app-dir-first by the
  documented order, yet System32 wins here.  This is the regression from the WORKING Jun 22 capture
  (proxy.log Jun 22 has full `[hook]`/`[anchor]`/DETACH = ours loaded then).
- **CONFOUND / open:** the test loader uses a RUNTIME `LoadLibrary` (modern mingw CRT may restrict the
  search to System32), while the GAME uses STATIC-import resolution — so the loader probe may not
  mirror the game.  And 64-bit PowerShell's `Process.Modules` on the 32-bit game is unreliable (showed
  "7 modules, no ddraw" which is likely a cross-bitness enumeration artifact, since a static ddraw
  import MUST be loaded at startup).  **Cannot confirm the game's actual ddraw path without observing a
  live game** (USER clicks Launch → read the game's loaded ddraw / the proxy.log).
- **Leading hypotheses:** (a) a Windows image-load policy that PREFERS System32 (Exploit Protection /
  Smart App Control / an ASR rule / a process mitigation) enabled since Jun 22 — would break app-dir
  shadowing for ALL the exe's imports (ddraw/dsound/winmm/dinput); (b) a DrvFs write-visibility race on
  the dropped DLL (weak — there's already ~100ms between the cp and the launch).  (a) ≠ fixable by
  re-targeting to a winmm/dsound proxy (same policy); it needs disabling the mitigation OR an injector
  (CreateProcess-suspended + remote-thread, i.e. back toward the Frida path the proxy avoided).
- **NEXT (needs USER):** one live capture where the USER clicks Launch and we read the proxy.log /
  the game's loaded ddraw module — decisive on whether the game loads ours vs System32.  Ask the USER
  what changed on Windows since ~Jun 22 (update / Defender / Exploit-Protection / Steam).

## Strict bar
A trace is frame-locked when `sync_diff` reports **0px** body divergence at every SETTLED tick AND
the wx/vel ramps match a STATE-FUL recording through the accel phases (camera-independent).  Screen-x
deltas during camera-FOLLOW motion are confounded (the camera pins Arche at ~270, hiding wx) and
during camera-CLAMP motion carry the recording's ~2.23-flip/tick aliasing — neither is a curve-fit
target.  Root-cause on the wx/vel axis (decompile + the field traces), never the aliased screen_x.
