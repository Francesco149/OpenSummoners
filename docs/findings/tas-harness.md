# TAS harness — 100% deterministic port↔retail trace diff

Goal (user, ckpt 47): run **100% deterministic** traces on both the port and
retail and diff them **side by side, frame for frame**, investigating any
pixel difference; extend the trace scene by scene (title → new-game →
prologue → beyond).

## The determinism model

Two independent processes must produce **identical frame streams** so a diff
is meaningful.  The sources of non-determinism and how each is pinned:

| source | port | retail (Frida harness) |
|--------|------|------------------------|
| RNG (`rand`/`srand`, LCG `DAT_008a4f94`) | seeds `OSS_RNG_DEFAULT_SEED 0x4f5347` at boot | `--seed-pin` writes the same value at the first phase-7 spawn |
| update cadence (updates per presented frame) | fixed timestep, **1 update / present** | `--lockstep` (see below) |
| wall-clock boot skew | n/a | absorbed by **anchors** (below) |

### Why retail needed the lockstep clock

The retail engine drives its **entire** update cadence off `GetTickCount`
deltas — no `timeGetTime`/`QueryPerformanceCounter`/`RDTSC`
(`winmain-and-bootstrap.md`).  Every scene loop (title `0x56aea0`, the
new-game modal pump `0x565d10`, the prologue cutscene `0x56cd20`) runs the
same 3-state `GetTickCount`-delta pace machine: it banks a time budget and
spends it in **16 ms (0x10) update slices**, presenting one update per slice.

The default turbo clock advances the virtual `GetTickCount` by `step_ms` on
**every** main-thread call.  With several calls per presented frame the pace
machine banks several 16 ms slices per Flip, so retail renders only ~1 of
every N update ticks (measured: subtitle anchor at flip **73** under turbo).
The port renders **every** update, so the two frame streams cannot be diffed
frame-for-frame — retail's presented frames are a 1/N subsample.

**`--lockstep`** freezes the virtual clock between Flips and banks exactly one
update quantum (`--lockstep-step-ms`, default 0x10) per present, so retail
renders **1 update / present** like the port.  A stall-breaker creep
(`--lockstep-epsilon-ms`, applied only after a long flipless burst of
`GetTickCount` calls) keeps asset-load busy-waits from hanging without
polluting the steady-state budget, so the 1:1 cadence stays exact.

Verified: under lockstep the subtitle anchor lands at flip **432** (vs 73
turbo), two independent runs are byte-identical AND each consecutive frame is
distinct (no 0-update/dup flips) → retail is exactly 1 update/present.

> **Retail is fully deterministic run-to-run** even under default turbo once
> the seed is pinned — two runs whose wall-clock seeds differ produced
> byte-identical frames through the dynamic intro AND the auto-demo.  Lockstep
> is needed not for determinism but for **port-matching cadence** (every
> update rendered) so the streams align frame-for-frame.

## Anchors

Under lockstep both sides run 1 update/present and march tick-for-tick
**within a scene**, but the **flip count** at which a scene/phase boundary is
reached differs per binary (boot skew + per-scene load cost).  A **TAS anchor**
is a named alignment point emitted on **both** sides at the same boundary,
stamped with the live flip and the RNG state.  The diff offsets retail's flip
axis onto the port's at each anchor; a mismatched RNG state pinpoints an
unaccounted `rand()` consumer.

| anchor | port site | retail site |
|--------|-----------|-------------|
| `subtitle_anim_start` | first phase-7 sparkle spawn (`drive_spawn_sparkle`) | first `FUN_0056c070` (the sparkle/seed-pin hook) |
| `newgame_enter` | `enter_newgame` | `FUN_00564780` entry |
| `prologue_enter` | `enter_prologue` | `FUN_0056cd20` entry |

Port emits `anchor: <name> flip=<N> rng=0x<hex>` on stderr; retail sends
`{kind:anchor,name,frame,rng}` (recorded in `run.json` summary `anchors` /
`anchor_rng`).

## Tools

- **`frida_capture.py --lockstep [--lockstep-step-ms N] [--lockstep-epsilon-ms N]`**
  — the deterministic retail clock + scene anchors.
- **`tools/tas_diff.py --anchor NAME --port-dir DIR --port-anchor N
  --retail-run DIR [--window W]`** — aligns a port capture set to a retail run
  on an anchor and reports per-tick `differ_px`.  Each port frame is matched
  to the **best** retail frame within ±W (so an occasional port 0-update
  DUPLICATE present — a cadence wrinkle that slips the offset by ±1 — is
  absorbed without hiding a real divergence; the chosen drift is reported).

### Recipe (self-serviceable)

```
# PORT to the prologue (trace into the game dir, bare filename):
printf '%s\n' '{"frame":620,"ids":[36]}' '{"frame":720,"ids":[3]}' \
  '{"frame":745,"ids":[3]}' '{"frame":800,"ids":[36]}' > "$OPENSUMMONERS_GAME_DIR/ng_trace.jsonl"
cp build/opensummoners-debug.exe /tmp/oss.exe
./build/opensummoners-launcher.exe --timeout-ms 60000 -- /tmp/oss.exe --hide-window \
  --frames 1600 --input-trace ng_trace.jsonl --capture-frames "801,..." --capture-dir=C:/osscap
# anchors print on stderr: subtitle_anim_start@437 newgame_enter@672 prologue_enter@801

# RETAIL to the prologue under lockstep (nav frames offset ~-5 vs the port):
printf '%s\n' '{"frame":615,"ids":[36]}' '{"frame":715,"ids":[3]}' \
  '{"frame":740,"ids":[3]}' '{"frame":795,"ids":[36]}' > /tmp/retail_ng_trace.jsonl
python3 tools/frida_capture.py --run-dir runs/X --exact-run-dir --lockstep \
  --input-trace /tmp/retail_ng_trace.jsonl --duration-ms 70000 --max-frames 300000 \
  --capture-frames "815,..."
# anchors: subtitle_anim_start@432 newgame_enter@667 prologue_enter@815

python3 tools/tas_diff.py --anchor prologue_enter --port-dir /mnt/c/osscap \
  --port-anchor 801 --retail-run runs/X
```

## Results

### Intro (`subtitle_anim_start`) — BIT-EXACT through the TAS pipeline
Port@438, retail@432 (offset 6).  All **28/28** phase-7 frames `differ_px=0`.
The only wobble: a single **port 0-update duplicate** present at tick 22
(port 459==460 byte-identical; retail all distinct) → realigned at drift −1 by
the diff window.  This re-proves the (already known bit-exact) intro through
the new frame-for-frame harness.

### Prologue cutscene (`prologue_enter`) — content bit-exact; two real findings

Port@801, retail@815.  Dense gem-rise cross-alignment: **63/64** port frames
have a `differ_px=0` retail match (only the scene-entry frame 0 lacks one —
the port's first present shows a partial gem, retail's is still black).  The
distinctive later frames (gem held, ticks 120/480) lock bit-exact at a
constant offset.  So the **gem / aura / scrolling-narration render is
faithful** — the cutscene was eyeball-verified at ckpt 46 and is now
**frame-for-frame bit-exact** in content.

Two divergences the anchors surfaced — **open RE threads, not render bugs**:

1. **The port skips the new-game → prologue transition.**  Port: confirm
   "Start Game" → `prologue_enter` in **1 flip**.  Retail: confirm → ~**20
   flips** (a fade-out / load of the new-game scene before `0x56cd20`).  This
   shifts retail's whole cutscene timeline ~14 flips later than the port's and
   is why a single constant anchor offset doesn't hold across the fade-in
   (the fade-level cadence is ±1 tick off during active animation; residuals
   246–662 px, i.e. one fade step).  The port's `app_flow` jumps straight from
   the new-game commit into `enter_prologue`; retail runs the real
   `0x564160`/`0x5642e0` transition in between.

2. **RNG desync at `prologue_enter`.**  newgame_enter: port & retail both
   `rng=0x404a0a8f` (match).  prologue_enter: port `0x404a0a8f`, retail
   `0x40d00581` (**differ**).  The retail transition (#1) consumes `rand()`
   the port does not.  The gem render is RNG-independent so this does not
   affect the gem frames, but any rand-driven cutscene effect downstream would
   diverge — this is the canonical "unaccounted rand() consumer between two
   anchors" the anchor RNG stamp is designed to catch.  Fix options: port the
   transition faithfully (consume the same rand stream), or force retail's RNG
   to the port's value at the anchor (a planned `--force-rng-at` knob).

## Next

- Port the new-game → prologue transition (`0x564160`/`0x5642e0`) so the port
  consumes the same rand stream and the cutscene timelines align at a constant
  offset — closes both open threads.
- `--force-rng-at <anchor>=<value>` retail knob to resync RNG at a boundary
  (isolates render parity from transition divergence while #1 is unported).
- Extend anchors + trace past the prologue into the game proper (`0x59ec30`)
  once the user opts to extend the trace in-game.
