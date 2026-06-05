<!--
  The ONE hand-edited status block.  tools/gen_port_ledger.py injects everything
  between the FRONT:BEGIN/END markers verbatim into docs/STATUS.md's "Current front"
  section, so STATUS can never drift from reality.  Update THIS when the active front
  moves; keep it a 60-second read.  Everything else in STATUS is derived from code.
  Port state / next-move detail belongs here or in HANDOFF.md, NOT in engine-quirks.md.
-->
<!-- FRONT:BEGIN -->
- **Phase:** Phase 4–5 — porting the **in-game town backdrop** render path (the first
  in-game visible frame), building toward a trace that plays 1:1 pixel-perfect frame by
  frame on both sides. Milestone map: `ROADMAP.md`. Mechanical next chip:
  `port-frontier.md`.
- **Where we are (ckpt 61):** the **decode → grid → geometry → draw-list** chain is
  CLOSED. `map_decode` places tiles in the runtime render grid (`map_grid`),
  `map_render_tile` reads the geometry, and `map_render_walk` + `draw_pool` (the
  27-layer draw-node pool, `0x4917b0`/`0x586010`) accumulate the per-frame draw-node
  list. All pure + host-tested: **806 pass / 0 fail / 6 skip**. Ledger **189/1490
  touched / 184 tested**. Both GUI builds clean; the new modules are in the `src`
  wildcard but **not yet called by `main.c`**.
- **Next move:** the **present pass** — walk the 27 layers, resolve each node's sprite,
  zdd-blit it (the consumer that turns the draw list into pixels) — OR wire
  `map_decode` + `map_render_walk` + present into `main.c`'s in-game scene and diff vs
  `runs/tas-ingame-1` anchored on `game_enter`. The camera/view object construction
  stays a rock (`cam[0x34..0x74]` are updated dynamically by gameplay scroll across many
  functions — no clean pure init); host tests use synthetic cameras (window math exact).
  Full writeup: `findings/in-game-intro.md`.
- **Tooling front:** **Phase-B B2 (the field-bearing flow trace) LANDED + live-verified**
  (`docs/plans/trace-tooling-phase-b.md`): `call_trace` now carries `seq` +
  `CALL_TRACE_BEGIN/FIELD/END`; the Frida agent reads same-named retail fields per
  `tools/flow/retail_fields.json`; `tools/flow_diff.py` names the first `[chain]`/`[data]`
  divergence (+ `--field-timeline`). First probe: `rng` (`DAT_008a4f94`) at the Flip
  (`0x5b8fc0`, the shared per-frame VA) — it already proved the title-sparkle RNG is
  **data-1:1** (both sides converge to `0x404a0a8f`), per-flip skew = the R3 title-pace
  (phase) pillar, not logic. Remaining Phase B: **B1** unified `scenario-test.py`, **B3**
  DDraw blit-command trace + `render_diff.py`.
- **Standing bar:** every divergence is `differ_px == 0` or a named/understood residual
  (`parity-ledger.md`); attribute to a pillar before suspecting logic
  (`parity-model.md`); seed-pinned both sides, compared by anchor/tick.
<!-- FRONT:END -->
