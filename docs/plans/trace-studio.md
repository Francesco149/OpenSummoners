# Trace Studio v1 — the scrub-and-mark viewer for port↔retail trace parity (RETIRED)

> **RETIRED (ckpt 128, USER directive) — SUPERSEDED by Trace Studio v2.**  This v1
> plan (the Python `tools/trace_studio.py` + `:8779` web SPA) is kept only as a design
> record.  The current studio is the native draw-stream pipeline: `.osr` capture on
> both sides → the native `tools/osr_view` scrub/drill/marks.  Do NOT build or run
> anything described here.  See **`docs/plans/trace-studio-v2.md`** and CLAUDE.md's
> trace-studio bullet.
>
> Status (2026-06-09, historical): v1 BUILT this checkpoint. The USER pulled this
> forward from Phase C ("gated on a controllable character") because input injection
> already works and visual checks are now frequent. Modeled on openrecet's
> `tools/trace_studio` (the proven loop); adapted to this project's harness.

## What it is

One command captures a scenario on BOTH targets (port + retail under
`--seed-pin --lockstep`), pairs the frames anchor-relative, and serves a browser
studio: **port | retail | diff videos scrubbing in lockstep**, a per-frame state
panel, a differ_px ribbon, and **marks/notes** — the USER's channel for flagging
divergences frame-by-frame. `apply` turns the marks into `worklist.md` for Claude
to chase with the existing drill-ins (`render_diff`/`flow_diff`).

The loop (same as openrecet): **capture → user scrubs + drops notes → Claude
reads the worklist → RE + fix the logic → re-capture the SAME session → user
re-checks** until the trace plays 1:1.

## Architecture (tools/trace_studio/, package; tools/trace_studio.py launcher)

- **drive/** — `port.py` runs the port via `tools/run-opensummoners.sh`
  (`--input-trace <working copy> --capture-all --capture-dir <win-staging>`),
  parses `anchor:` stderr lines; `retail.py` runs `tools/frida_capture.py`
  (`--input-trace --capture-frames all --no-turbo --lockstep --seed-pin`);
  `runner.py` drives both concurrently.
- **analysis/pairing.py** — THE alignment core. Segments the two flip axes on the
  shared TAS anchors (`subtitle_anim_start`, `newgame_enter`, `prologue_enter`,
  `game_enter`; boot=flip 0), pairs tick-for-tick within each segment with a
  **sticky ±drift best-match** (the port's occasional 0-update duplicate frame —
  see `tas_diff.py`), and emits ordinal-named frames: `frame_<ordinal>.png` is
  THE SAME captured moment on every side (port/, retail/, diff/). Early-out: a
  pair that's differ_px==0 at the sticky drift skips the search.
- **analysis/state.py** — `state.jsonl`: one row per ordinal `{frame, seg,
  port:{flip,…}, retail:{flip,sim_tick,…}, drift}`; merges per-flip flow-trace
  fields when the session was captured `--call-trace`.
- **analysis/verdict.py** — anchor-RNG verdict (port vs retail `rng=` at every
  shared anchor — free, no call trace needed) + `flow_diff --verdict` when call
  traces exist.
- **transport/encode.py** — ffmpeg all-intra h264 (keyint=1) → frame-exact
  browser seeking; one mp4 per panel.
- **edits/** — mark registry (`note`/`feature`/`rng`/`phase`, all worklist
  kinds; none auto-apply — our RNG is globally seed-pinned, unlike openrecet's
  per-trace pins) + `apply.py` → `worklist.md` with frame context.
- **server/** — the http server (ranged mp4 + JSON API + capture jobs) and the
  SPA in `tools/trace_studio_web/` (htm+preact, adapted from openrecet).
- **Sessions** live in `runs/trace-studio/<name>/` (gitignored): `session.json`,
  `{port,retail,diff}/frames/`, `{port,retail,diff}.mp4`, `state.jsonl`,
  `edits.jsonl`, `worklist.md`, `anchors.{port,retail}.jsonl`,
  `edit.trace.{port,retail}.jsonl` (the editable working input traces),
  `port.log`/`retail.log`.

## Coordinate contract

`ordinal` (0-based viewer index) = the i-th PAIRED frame. Frame FILES on all
three sides are named by ordinal; `state.jsonl`/`diff.per_frame`/marks key by
ordinal; the per-side absolute flips live in the state row. Anchors map to
ordinals in `manifest.anchors_ordinals` (the UI's anchor track).

## Known phase caveat

The TITLE segment does not pair tick-for-tick (parity-ledger R3: retail renders
each title update ~2.2×) — title-region redness in the ribbon is the documented
R3 residual, not a regression. Segments from `newgame_enter` on pair 1:1 under
lockstep.

## Deferred (v2+)

- Port-side live RECORDER (play the port, record raw inputs → scenario), like
  openrecet's F2 record panel — needs port interactive input capture; lands with
  the controllable-character milestone.
- Anchor-gated segtrace ops (`{wait:ANCHOR}` input traces instead of per-side
  absolute-flip traces) — would make one trace drive both sides; today the
  scenario carries `trace-port.jsonl`/`trace-retail.jsonl` separately timed.
- `--call-trace` default-on once the field spec is cheap enough over a full
  intro capture.
- Drill (re-capture a sub-window dense) — ours captures dense already; becomes
  relevant with longer traces/strides.
