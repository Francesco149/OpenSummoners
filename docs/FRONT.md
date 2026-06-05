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
- **Where we are (ckpt 64):** the **decode → grid → geometry → draw-list → present**
  chain is CLOSED, and the **camera/view object is now RE'd + the first-frame value
  live-probed** (ckpt 64 — the last non-wiring blocker, resolved). `map_decode` places
  tiles in the runtime render grid (`map_grid`), `map_render_walk` + `draw_pool` (the
  27-layer pool, `0x4917b0`/`0x586010`) accumulate the draw list, **`map_present`**
  (`0x48eac0` + projector `0x490b90`) walks the 27 layers → mode-3 backdrop → zdd blits.
  **Camera:** `view = *(room_state+0x104c)` = `operator_new(0x78)` (the 0x4017d0 ctor);
  the room-entry init (`586010:854-872` + `587d30`) is portable (`map_render_camera_init`),
  and the opening town's first frame is the live-verified constant `MAP_RENDER_CAM_TOWN_3F2`
  (`+0x60=128000`/`+0x5c=12800`, vp 64000×48000; window cols 39-60 / rows 3-18). All pure
  + host-tested: **821 pass / 0 fail / 6 skip**. Ledger **191/1490 touched / 186 tested**.
  Both GUI builds clean; the new modules are in the `src` wildcard but **not yet called by
  `main.c`**.
- **Next move:** **real town-backdrop pixels — only the wiring remains** (the camera is no
  longer a blocker). Wire `map_decode` + `map_render_walk` + `map_present(cam=MAP_RENDER_CAM_TOWN_3F2)`
  into `main.c`'s in-game `game_render`, with a real sprite resolver (`0x418470` /
  `&DAT_008a760c`), the EXE-NULL banks `0x570-0x572`, and the `0x586010` sim slice that
  populates the grid + builds the `view+0x54` layer table; then diff vs `runs/tas-ingame-1`
  anchored on `game_enter` (golden first town frame ~flip 1150). Deferred: present modes
  0/1/2 (actor draws) + the spawn-snap/intro-pan (PORT-DEBT `ingame-camera-snap`). Full
  writeup: `findings/in-game-intro.md` "The camera/view object".
- **Tooling front:** **Phase-B B2 (the field-bearing flow trace) LANDED + live-verified**
  (`docs/plans/trace-tooling-phase-b.md`): `call_trace` now carries `seq` +
  `CALL_TRACE_BEGIN/FIELD/END`; the Frida agent reads same-named retail fields per
  `tools/flow/retail_fields.json`; `tools/flow_diff.py` names the first `[chain]`/`[data]`
  divergence (+ `--field-timeline`). First probe: `rng` (`DAT_008a4f94`) at the Flip
  (`0x5b8fc0`, the shared per-frame VA) — it already proved the title-sparkle RNG is
  **data-1:1** (both sides converge to `0x404a0a8f`), per-flip skew = the R3 title-pace
  (phase) pillar, not logic. Ckpt 64 added a `src:"chain"` field (global root + pointer
  hops, e.g. `*(*(0x8a9b50)+0x104c)+off`) and used it to live-probe the in-game camera
  (the `cam_*` fields). Remaining Phase B: **B1** unified `scenario-test.py`, **B3**
  DDraw blit-command trace + `render_diff.py`.
- **Standing bar:** every divergence is `differ_px == 0` or a named/understood residual
  (`parity-ledger.md`); attribute to a pillar before suspecting logic
  (`parity-model.md`); seed-pinned both sides, compared by anchor/tick.
<!-- FRONT:END -->
