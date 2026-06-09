# Trace Studio — how to run the scrub-and-mark loop

Architecture/design: `docs/plans/trace-studio.md`. This is the operational
cheat-sheet. Everything runs inside `nix develop`.

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
