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
- **Where we are (ckpt 65): REAL IN-GAME PIXELS.** The decode → grid → walk → present
  pipeline is **composed (`town_render.{c,h}`) + WIRED into `main.c`**, and the port now
  renders the opening **town of Tonkiness backdrop** — the half-timbered house, the vine
  trellis, the stone-block walls, ivy + grass — the **same assets at the matching gameplay
  scale as the retail golden** (cross-checked vs golden flip 1800). Live-verified: the map
  **DATA 1022** (152936 B "MSD_SOTES_MAPDATA", 88×19×3) loads from the *packed* `sotes.exe`
  `.rsrc` (`LoadLibraryExA AS_DATAFILE`); `game_render` walks `town_render` through the
  first-frame camera `MAP_RENDER_CAM_TOWN_3F2`; the three engine globals are real callbacks
  (sprite resolve `ar_pool_get_slot`+`ar_sprite_slot_frame`, bank dims, blit
  `zdd_object_blt_clipped`). **827 pass / 0 fail / 6 skip** (+6 town_render). Ledger
  **191/1490 touched / 186 tested** (pure composition, no new `FUN_`). Both GUI builds clean.
- **NOT `differ_px==0` yet — named residuals, all deferred layers** (NOT logic bugs): the
  parallax sky/mountain far-plane + foreground trees + dialogue/caption overlay (`0x5a00c0`,
  PORT-DEBT `ingame-nontile-layers`); the NPC actors (present modes 0/1/2, PORT-DEBT
  `present-actor-modes`); and retail's **zoomed-out intro establishing shot** at the hold
  (flip 1150 is a whole-town vista that zooms to 1:1 by ~1800 — the camera scale field
  wasn't in the ckpt-64 probe; PORT-DEBT `ingame-establishing-zoom`). The backdrop TILE
  layer itself is asset+scale-correct.
- **Next move:** the smallest visible win on the now-proven tile layer — the **parallax
  far-plane** (sky/mountain full-screen backdrop), then the **actor renderers** (`0x491ae0`
  et al. → present modes 0/1/2), then the **dialogue overlay** (glyph pipeline). First pin
  the **establishing-shot/zoom** relationship (so port + golden share a camera) to unlock a
  flip-anchored full-frame diff vs `runs/tas-ingame-1`. Full writeup:
  `findings/in-game-intro.md` "The backdrop pipeline WIRED".
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
