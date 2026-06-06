# Phase B — unified harness + divergence tracing

> **Status (2026-06-06):** **B2 + B3 LANDED + live-verified.** Only B1 (unified
> `scenario-test.py`) remains. B3 (the DDraw blit-command + state trace,
> `render_diff.py`, the `render_id` cross-side identity + decode fingerprint) is
> done and verified cross-side on real captures — see `findings/ddraw-blit-trace.md`.
> See the "Status" note under each deliverable below.

The rigor scaffolding (CLAUDE.md, FRONT.md, parity-model, port-debt, ods cross-ref/proofs)
landed 2026-06-05. This is the **tooling** half: bring the divergence-chasing loop up to
the openrecet standard — a **minimal tooling surface** (one command, few flags) and a
trace that **pinpoints** the exact divergence instead of just diffing pixels.

The north star: a trace **plays 1:1 pixel-perfect frame by frame on both sides, and the
API/call traces match**, modulo documented benign deviations + occasional ≤1-LSB texture
sampling noise (`parity-model.md`).

## Where we are today (the sprawl to collapse)
Determinism is already strong: anchors on both sides, RNG seed pin (`OSS_RNG_DEFAULT_SEED`
/ retail `--seed-pin`), `--lockstep`, `tas_diff.py` anchor-aligned pixel diff with drift
search. But the surface is fragmented across `run-opensummoners.sh`, `run-retail.sh`,
`frida_capture.py`, `tas_diff.py`, `call_trace_diff.py`, and a pile of one-off probe flags
(`--cursor-probe`/`--fade-probe`/`--pace-probe`/`--seed-pin`/`--textout-probe`/
`--menu-trace`/`--box-probe`/`--rand-probe`, catalogued per-ckpt in PROGRESS.md). And the
call trace is **function-coverage only** — it says *which* engine functions each side
reached, not *which field diverged* or *which blit was wrong*.

## Deliverables, in order

### B1. `tools/scenario-test.py <scn> --target {port|retail|both}` — the one command
Owns the whole loop: input/anchor replay, RNG/seed pin, `--lockstep`, resolution pinning,
frame alignment, and capture — collapsing the scripts + probe flags into one entry point
(the existing scripts become its primitives, not the user surface). Scenarios live in
`tests/scenarios/`. Mirrors openrecet's `scenario-test.py`. During this work, **catalogue
the surviving probe flags into `docs/parity-harness.md`** (the per-ckpt PROGRESS notes get
consolidated here as they fold into `scenario-test.py`).

### B2. Field-bearing flow trace — the LOGIC drill-in ✅ DONE (live-verified 2026-06-05)
openrecet's single most effective tool: names the **first call whose inputs matched but
whose output/state diverged**. Ported to SotES:
- ✅ `src/call_trace.{c,h}` extended from `CALL_TRACE_ENTER` to **`CALL_TRACE_BEGIN/FIELD/END`**
  (+ `_STUB`, + `I32/U32/F32/HEX`) with a per-frame `seq` stamped on every row. 4 host tests.
- ✅ `tools/frida/opensummoners-agent.js` reads the **same-named fields** from retail per
  `tools/flow/retail_fields.json` (`src: global | arg | argderef`; `i32|u32|f32|hex`; `retval`
  is an onLeave TODO), with a per-Flip `seq` mirroring the port. `frida_capture.py` loads the
  spec, `--field-spec[-only]` auto-hooks its VAs (the bounded mode); the batch writer passes
  `seq`/`f` through verbatim.
- ✅ `tools/flow_diff.py`: aligns the per-frame chain by `seq`, classifies the first divergence
  as `[chain]` / `[data]`; `--field-timeline` is the per-field state localizer. 9 tests.
- **First probe (seed):** `rng` (the LCG word `DAT_008a4f94`) at the **Flip** (`0x5b8fc0`) —
  the shared once-per-frame VA on both sides. NB the title runner `FUN_0056aea0` keeps its
  do/while loop INTERNAL (onEnter fires once at scene entry, not per frame), so it is the
  WRONG cross-side VA; the Flip is right. Coverage grows from here with the sweep.
- **First result (the tool working):** the rng field-trace shows the title sparkle's RNG
  consumption is **data-1:1** — port and retail converge to the identical end state
  `0x404a0a8f` (same total draws) — with the per-flip divergence attributable to the
  **title-pace skew (parity-ledger R3, the phase pillar)**, NOT logic. A textbook
  data-1:1-vs-observed-divergence call. A pace-aware (anchor+rate) alignment, or moving the
  probe to a pace-invariant point, is the next refinement when this gets chased to differ_px.

### B3. DDraw blit-command + state trace — the RENDER-STREAM drill-in ✅ DONE (live-verified 2026-06-06)
SotES renders via a **DirectDraw 7 software blitter** (not D3D8), so the render-stream
analog of openrecet's `d3d-trace` is a **blit-command log**. Landed — full writeup +
how-to in **`docs/findings/ddraw-blit-trace.md`**:
- ✅ `src/render_id.{c,h}` — the cross-side identity backbone: a cel→`(resource_id, frame)`
  registry (openrecet's `tex_name` trick — drop the allocation-dependent pointer, key on
  the load-stable asset name) **plus** `dhash`, an FNV-1a fingerprint of the DECODED sheet
  pixels. The dhash is the improvement over openrecet's name-only scheme — for a software
  blitter the pixels are CPU-accessible at decode time, so it catches the RIGHT sprite
  decoding to the WRONG pixels (the palette/24bpp residual class), not just the wrong
  sprite. Host-tested (7).
- ✅ Port emits at the 5 blit primitives (`0x5b9a40`/`_9b70`/`_9ae0`/`_9bf0`/`_bd550`) via
  `zdd_emit_blit` — identity + raw geometry + the DDraw state (colorkey, KEYSRC arm, blend
  mode). Rides the existing `call_trace` transport (no second emitter).
- ✅ Retail mirror in the Frida agent: `installRenderIdHook` (the resolver `0x418470`
  registry) + two new field sources `renderid`/`thisderef` (each auto-installs, no ad-hoc
  flag); the blit VAs in `tools/flow/retail_fields.json`.
- ✅ `tools/render_diff.py` aligns each frame's blit sequence by `(va, res, frame)` and
  classifies the first divergence: `[sprite]`/`[decode]`/`[rect]`/`[state]`. Host-tested (9).
- **Verified cross-side:** retail emits the IDENTICAL `resource_id` (0x91b) as the port for
  the title background; the rects/state read correctly off ECX+args; render_diff names every
  blit by identity. **Next layers:** the retail-side decode-hash (so `[decode]` fires
  cross-side), the cdecl `0x5bd550` retail spec, and a same-scene aligned in-game diff.

## The loop each tool enables
For every divergence: **pinpoint** (flow_diff → the call / render_diff → the blit) →
**attribute to a pillar** (`parity-model.md`: logic / phase / RNG / inputs) → **fix the
logic** (RE the responsible code, don't curve-fit) → **re-verify to `differ_px == 0`** →
**log** the quirk (`engine-quirks.md`, retail-only) + the parity-ledger row, and retire any
`port-debt.md` shortcut the fix closes. This is exactly the cadence openrecet ran across
its 2026-06-05 fix series (menu-bright denormal, select-pulse −128, cos-vs-sin box halo,
COLOROP ADDSIGNED) — each a one-divergence chip backed by a quirk + ledger entry.

## Optional follow-ons (only if a chase needs them)
- A `phase_probe.py` analog (ALIGNED / CONST-OFFSET / DRIFT verdict) if phase attribution
  by hand becomes a bottleneck — the model is already written (`parity-model.md`).
- Git hooks (`commit-msg` co-author trailer + `pre-commit` ledger-regen/test gate) to make
  the manual conventions enforced, as openrecet does. Currently manual here.
