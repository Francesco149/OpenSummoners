# TAS framework — deterministic input traces + golden-frame parity

Mirrors the openrecet TAS harness (`/opt/src/openrecet/tools/scenario-test.py`
+ `src/input_trace.{c,h}` + `tests/scenarios/`) for OpenSummoners. Goal:
**structural parity AND pixel accuracy with retail** on a scripted run —
specifically *click through NEW GAME → first in-game frame rendered* — driven
by a **deterministic input trace** so every run is reproducible and diffable.

## The openrecet model (what we are mirroring)

| piece | openrecet | role |
|---|---|---|
| sparse input trace | `src/input_trace.h` JSONL `{frame,buttons}` | one line per button-mask *change*; holds between |
| scenario dir | `tests/scenarios/<name>/{scenario.yaml,trace.jsonl,golden/,golden-retail/}` | committed config+trace; gitignored goldens |
| port replay | `--input-trace-replay` binds the mask into the engine slot pre-sim | deterministic port run |
| retail capture | Frida agent: hook Present → readback surface → BMP; **inject** the trace into the input poll | deterministic retail run |
| driver | `scenario-test.py --target {port,retail,both}` | runs, captures, diffs, side-by-sides |
| visual | `tools/push_comparison.py` → llm-feed | port\|retail atlas + amplified diff |

Bit-exact diff *within* a target (deterministic input + pinned RNG + virtual
clock). Cross-target (retail vs port) is **not** bit-exact — that is the
side-by-side + amplified-diff review surface on llm-feed.

## OpenSummoners adaptation

Engine differences from Recettear:

- **Renderer:** DirectDraw7 (not D3D8). 640×480 **16bpp RGB565**, primary +
  back-buffer flip chain. Frame anchor already in place: the Flip dispatcher
  `FUN_005b8fc0` (`g_flip_frame`).
- **Input:** not a flat button mask but a **64-entry ring of event-record
  pointers** at `input_mgr+0x0c..0x108`; each record `{id, ts(GetTickCount),
  flag}`. The poll (`0x43c110`) scans newest-first within a 100 ms window and
  consumes on read. See `docs/findings/input.md`.

### Retail side (where ground truth lives *now*)

The port cannot render yet (render bridges stubbed, `main.c` doesn't drive
`title_scene_step`), so **all current ground truth comes from retail under
Frida**. Two new agent capabilities:

1. **Frame capture** (`opensummoners-agent.js`): at the Flip hook, on a frame
   whitelist, `GetDC` the back buffer (or primary), `BitBlt` into a 24bpp
   top-down DIB (GDI handles the RGB565→BGR conversion for free), read the
   bits, `send()` them to the driver, which writes `frame_NNNNN.bmp`.
   Capture runs **inline on the engine thread** (Interceptor onEnter), so no
   cross-thread DDraw hazard.
2. **Input injection**: on a hidden, unfocused window DInput produces nothing,
   so the ring is ours to fill. Resolve `input_mgr` by hooking the poll
   consumer `0x43c110` (ecx). Per engine frame, for each button pressed in the
   trace, write a synthetic record `{id, GetTickCount(), flag=1}` and store its
   pointer in the newest ring slot. Feeds the poll, `input_any_fresh_press`
   (skip-splash), and the menu latch identically to a real press.

Same sparse `{frame,buttons}` JSONL schema as openrecet; `buttons` is an OR of
title button ids (`0x01` up, `0x02` down, `0x03` left, `0x04` right,
`0x24` back; confirm id TBD — recover from the menu latch / a recorded trace).

### Port side (latent until rendering lands)

- `src/input_trace.{c,h}` — port openrecet's pure-C sparse replay/record;
  host-tested now, wired into `main.c` once it drives `title_scene_step`.
- Port-side frame capture = dump the DDraw surface after `zdd_present`.

Both are **blocked on milestone-0 rendering** (HANDOFF Next move #1/#2). The
retail-side pipeline is built first; it produces the goldens the port targets.

## Scenario layout

```
tests/scenarios/<name>/
    scenario.yaml      # description, max_frames, capture_frames, rng_seed
    trace.jsonl        # sparse {frame,buttons}
    golden-retail/     # gitignored; --bless populates via Frida
        frame_NNNNN.bmp
    golden/            # gitignored; port frames (once rendering lands)
```

First scenarios: `title-idle` (boot → title, no input), then
`new-game-through` (the click-through to the first in-game frame).

## Commands (live = Frida on the Windows host, self-serviceable)

**Always launch via `tools/run-retail.sh`, never `frida_capture.py` directly.**
`frida_capture.py`'s default `--exe vendor/original/sotes.exe` is the
Steam-DRM-**packed** image — spawning it trips Steam DRM (a popup + the real
game relaunching as a separate, unhooked process → 0 Flips). `run-retail.sh`
drops the Steamless-**unpacked** exe next to the game DLLs, spawns that, and
cleans up the copy + kills the child on exit. Extra flags pass through `"$@"`.

```sh
# retail frame capture (no-turbo MANDATORY — turbo freezes the splash, quirk #29)
OPENSUMMONERS_DURATION_MS=20000 tools/run-retail.sh --no-turbo \
    --capture-frames "60,200,400,700,1000,1300,1600,1900" \
    --run-dir runs/title-idle --exact-run-dir
# → runs/title-idle/frames/frame_NNNNN.png   (640x480, lossless PNG)

# with deterministic input injection
OPENSUMMONERS_DURATION_MS=20000 tools/run-retail.sh --no-turbo \
    --input-trace tests/scenarios/new-game-through/trace.jsonl \
    --capture-frames "..." --run-dir runs/new-game-through --exact-run-dir
```

Push results to llm-feed (`$P montage`/`$P comparison`) for visual review;
zoom ≥4× for pixel-level diffs.

## Status

- [x] retail frame→PNG capture in agent + driver (`--capture-frames`) — validated
- [x] retail input injection in agent + driver (`--input-trace`) — validated:
      deterministic NEW GAME click-through (title → difficulty menu → Start
      Game → opening cutscene) reproduces frame-for-frame
- [x] scenario layout + `title-idle` + `new-game-through` traces
- [ ] **prologue → first playable map** — the opening cutscene (stone +
      narration) is a timed/advance cutscene; capture from a recorded human
      trace (distil to sparse) or RE the prologue sequencer
      (`docs/findings/new-game-flow.md`)
- [ ] port-side input_trace.{c,h} (latent — buildable/testable now)
- [ ] port-side render + frame capture (blocked on milestone-0 rendering)

See `docs/findings/new-game-flow.md` for the recovered scene sequence and the
per-menu button-id maps (engine-quirks #42/#43).
