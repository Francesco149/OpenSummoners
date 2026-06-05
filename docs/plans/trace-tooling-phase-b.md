# Phase B — unified harness + divergence tracing (tracked, not yet started)

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

### B2. Field-bearing flow trace — the LOGIC drill-in *(built first, per decision)*
openrecet's single most effective tool: names the **first call whose inputs matched but
whose output/state diverged**. Port it to SotES:
- Extend `src/call_trace.{c,h}` from `CALL_TRACE_ENTER` to **`CALL_TRACE_BEGIN/FIELD/END`**
  with a per-frame `seq` — each ported function declares the fields that carry its state.
- Teach the Frida agent (`tools/frida/opensummoners-agent.js`) to read the **same-named
  fields** from retail (a field-spec JSON, like openrecet's `retail_fields.json`).
- Add `tools/flow_diff.py`: align the per-frame call chain by `seq`, classify the first
  divergence as `[chain]` (call present on one side only) or `[data]` (inputs matched,
  output diverged). Coverage grows with the sweep.

### B3. DDraw blit-command trace — the RENDER-STREAM drill-in *(after B2)*
SotES renders via a **DirectDraw 7 software blitter** (not D3D8), so the render-stream
analog of openrecet's `d3d-trace` is a **blit-command log**: every blit through the hot
path (`0x5bd550` orchestrator → `0x5bd680` software alpha blitter — the per-frame draw
primitive confirmed in `parity-harness.md`) with src/dst rect, mode, and surface id, on
both sides, plus a `tools/render_diff.py` that names the **first divergent blit** (which
draw is wrong, before chasing why). The draw-node layer pool (`draw_pool`, ckpt 61) is the
natural emit point on the port side.

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
