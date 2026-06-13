# Trace Studio (v1) — RETIRED (ckpt 128, USER directive)

> **Do not run this loop.** The v1 web studio (`tools/trace_studio.py` capture +
> the `:8779` serve) is superseded by **Trace Studio v2**: `.osr` draw-stream
> capture on both sides (`tools/capture_proxy/run_proxy.sh` retail,
> `opensummoners.exe --osr-emit` port) reviewed in the native `tools/osr_view`.
> See `docs/plans/trace-studio-v2.md`.  Old sessions in `runs/trace-studio/`
> stay read-only — their navs/traces remain the proven scenario inputs.

Architecture/design: `docs/plans/trace-studio.md`. This was the operational
cheat-sheet, kept for reading old sessions. Everything runs inside `nix develop`.

## The loop

1. **Capture** a scenario on both targets into a session:
   ```sh
   nix develop --command python3 tools/trace_studio.py capture in-game-intro \
       [--session NAME] [--port-frames 2600] [--retail-frames 3200] [--call-trace]
   ```
   Drives the port (`run-opensummoners.sh --capture-all`, BMPs staged under
   `C:/Users/…/Temp/oss-trace-studio/` — the exe can't fopen WSL paths) and
   retail (`frida_capture --no-turbo --lockstep --seed-pin --capture-frames all
   --max-flips N`) **concurrently**, then pairs the two flip axes
   anchor-segmented (boot / `subtitle_anim_start` / `newgame_enter` /
   `prologue_enter` / `game_enter`) with a sticky ±drift best-match, builds
   amplified diffs + all-intra scrub mp4s + `state.jsonl` + the verdict.

2. **Serve** the studio and review in the browser:
   ```sh
   nix develop --command python3 tools/trace_studio.py serve --session NAME
   #  → http://localhost:8779/?session=NAME   (binds 0.0.0.0 for the Windows browser)
   ```
   Three panels (port | retail | diff) scrub in lockstep. ←/→ ±10 · ,/. ±1 ·
   Home/End · 1/2/3 toggle panels. The ribbon is per-frame `differ_px`
   (black = bit-exact); the anchor track jumps to segment starts; the state
   sidebar shows seg / per-side flips / drift / sim_tick (+ flow fields on
   `--call-trace` sessions).

3. **The USER flags divergences**: drag a box on a panel (attaches the region),
   type a note, hit `✎ divergence note` (or feature/rng/phase). Marks render as
   retail|port|white-diff crop thumbnails (click to zoom).

4. **Hand-off**: `✎ worklist` (or `tools/trace_studio.py apply NAME`) renders
   the marks into `runs/trace-studio/NAME/worklist.md` with full context
   (segment, anchor-relative position, BOTH absolute flips, differ_px, box).
   Claude chases each item with the drill-ins (`render_diff` = which blit,
   `flow_diff` = which logic), fixes the logic, then:

5. **Re-capture** (browser button or CLI) — `--only port` re-runs just the
   port with the current build against the cached retail capture (the fast
   fix loop). Marks and working-trace edits survive re-captures.
   ```sh
   nix develop --command python3 tools/trace_studio.py recapture NAME --only port
   ```

Also: `pair NAME [--drift-window K] [--amp X]` re-runs pairing/videos/verdict
from the existing raw captures (no re-drive); `sessions` lists sessions.

## The SIM-TICK axis (ckpt 105) — attribute on ticks, not flips

Both sides' captured frame names carry the deterministic sim-tick axis:
`frame_<flip>_t<tick>.png`.  The tick = the **easer-call count** (retail: the
Frida agent counts `0x43d1d0` onEnter; port: `g_sim_tick_count` in `main.c`,
incremented in the same gated easer block — 0 until the in-game scene pumps).
`state.jsonl` carries `port.sim_tick` + `retail.sim_tick` per paired frame, the
viewer shows both (red when they differ at the pair), and worklist rows print
both ticks — **a tick MISMATCH at a mark is the phase pillar (pairing offset)
at a glance; a tick MATCH with pixels differing is a real logic lead.**

Chasing a mark, compare at FORCED tick-equality (pull both `_t` maps, diff
port tick T against retail T+dt for dt around 0), not at the pairing's choice —
the sticky drift is pixel-driven and wanders ±1-3 ticks through content-quiet
stretches.  Two footguns measured on intro-1:
- **Retail coalesces ticks**: mostly 2 flips/tick but with 1-flip ticks and
  un-presented ticks (intro-1 never presented ticks 41/491).  Flip-axis trigger
  calibrations absorb these as ±1-2 tick errors (engine-quirk #99).
- **Fade phases plateau**: alpha-ramp indices quantize (~2.5 ticks/index for
  the banner), so a dt scan returns differ_px==0 over 2-3 consecutive dt.
  Calibrate fade timing off the per-present VALUE sequence (each distinct
  level's first tick), never a dt minimum.

## Reading the output honestly

- **boot/title segment redness is documented** — parity-ledger R3: retail
  renders each title update ~2.2× wall-clock; under lockstep the intro phase
  timings still don't pair tick-for-tick. Not a regression.
- **anchor-rng DESYNC before `game_enter` is expected** on scenarios whose nav
  skips the title sparkle (quirk #77 — the retail boot-seed pin lands at the
  first `0x56c070` spawn). The town re-pins at `game_enter` on both sides.
- **drift ≠ bug**: ±1 steps absorb the port's duplicate-frame cadence wrinkle;
  a CONSTANT lock (e.g. prologue at −7) measures the per-side anchor→content
  arm offset. A clean segment locks and goes black; a content-divergent
  segment (e.g. the town pre-dialogue-port) hunts — port the missing content
  first, then the lock appears.
- A session is `runs/trace-studio/<name>/` (gitignored): `session.json`,
  ordinal-named `{port,retail,diff}/frames/frame_<k>.png` (same k = same
  captured moment), `{port,retail,diff}.mp4`, `state.jsonl`, `edits.jsonl`
  (marks), `worklist.md`, `anchors.{port,retail}.jsonl`, working traces
  `edit.trace.{port,retail}.jsonl`, `port.log`/`retail.log`.

## Footguns (learned on the first live captures)

- **Captures collide**: one retail game at a time — a second boot dies with
  "Game is already running." The drive pre-kills leftovers THROUGH FRIDA
  (taskkill gets Access-denied: the game is a child of the elevated
  frida-server). Don't run a CLI capture while a browser re-capture runs.
- **WSL interop context**: the port exe launch can fail with
  `UtilAcceptVsock … accept4 failed 110` from detached/sandboxed contexts.
  Run captures from a normal interactive shell (the studio server inherits a
  good context from the shell that started `serve`).
- **Scenario traces are per-side** (`trace-port.jsonl` / `trace-retail.jsonl`,
  absolute flips on each side's own axis) — edit the SESSION's working copies
  in the studio, not the committed scenario; `--reset-traces` rebuilds them.
